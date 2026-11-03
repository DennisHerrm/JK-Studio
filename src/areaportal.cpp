// areaportal.cpp - siehe bhed/areaportal.h
#include "bhed/areaportal.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <sstream>

namespace bhed {

void AreaPortale::anlegen(int gebiete) {
    zahl_ = std::max(gebiete, 0);
    portale_.assign(static_cast<std::size_t>(zahl_) * static_cast<std::size_t>(zahl_), 0);
    flut_.resize(static_cast<std::size_t>(zahl_));
    for (int i = 0; i < zahl_; ++i) { flut_[static_cast<std::size_t>(i)] = i; }
}

void AreaPortale::setze(int a, int b, bool offen) {
    if (a < 0 || b < 0 || a >= zahl_ || b >= zahl_) { return; }
    const auto n = static_cast<std::size_t>(zahl_);
    int& ab = portale_[static_cast<std::size_t>(a) * n + static_cast<std::size_t>(b)];
    int& ba = portale_[static_cast<std::size_t>(b) * n + static_cast<std::size_t>(a)];
    ab = std::max(0, ab + (offen ? 1 : -1));
    ba = std::max(0, ba + (offen ? 1 : -1));
    // CM_FloodAreaConnections
    std::fill(flut_.begin(), flut_.end(), -1);
    int nummer = 0;
    std::vector<int> stapel;
    for (int s = 0; s < zahl_; ++s) {
        if (flut_[static_cast<std::size_t>(s)] >= 0) { continue; }
        ++nummer;
        stapel.push_back(s);
        flut_[static_cast<std::size_t>(s)] = nummer;
        while (!stapel.empty()) {
            const int at = stapel.back();
            stapel.pop_back();
            for (int k = 0; k < zahl_; ++k) {
                if (portale_[static_cast<std::size_t>(at) * n + static_cast<std::size_t>(k)] > 0 && flut_[static_cast<std::size_t>(k)] < 0) {
                    flut_[static_cast<std::size_t>(k)] = nummer;
                    stapel.push_back(k);
                }
            }
        }
    }
}

bool AreaPortale::verbunden(int a, int b) const {
    if (a < 0 || b < 0 || a >= zahl_ || b >= zahl_) { return false; }
    return flut_[static_cast<std::size_t>(a)] == flut_[static_cast<std::size_t>(b)];
}

int punktGebiet(const BspGeometry& g, const float p[3]) {
    const int blatt = g.leafAt(p);
    return blatt >= 0 && static_cast<std::size_t>(blatt) < g.leafs.size() ? g.leafs[static_cast<std::size_t>(blatt)].area : -1;
}

AreaPaar kastenGebiete(const BspGeometry& g, const float mins[3], const float maxs[3]) {
    AreaPaar aus;
    std::vector<int> blaetter;
    blaetterImKasten(g, mins, maxs, blaetter);
    for (const int b : blaetter) {
        const int area = g.leafs[static_cast<std::size_t>(b)].area;
        if (area == -1) { continue; }
        // "doors may legally straggle two areas, but nothing should ever need more than that"
        if (aus.a != -1 && aus.a != area) {
            aus.b = area;
        } else {
            aus.a = area;
        }
    }
    return aus;
}

Fernebene fernebeneFuer(const float auge[3], const float vorn[3], float distanceCull) {
    Fernebene f;
    for (int k = 0; k < 3; ++k) { f.normale[k] = -vorn[k]; }
    f.abstand = auge[0] * f.normale[0] + auge[1] * f.normale[1] + auge[2] * f.normale[2] - distanceCull * 1.02F;
    return f;
}

bool ganzDahinter(const Fernebene& f, const float mins[3], const float maxs[3]) {
    // BoxOnPlaneSide == 2: auch die Ecke, die am weitesten auf der Seite der Normalen liegt, ist dahinter
    float naechste = 0.0F;
    for (int k = 0; k < 3; ++k) { naechste += f.normale[k] * (f.normale[k] >= 0.0F ? maxs[k] : mins[k]); }
    return naechste < f.abstand;
}

FernStand fernStand(const BspGeometry& g, const std::vector<FlaechenKasten>& kaesten, const Fernebene& f) {
    FernStand aus;
    if (g.models.empty()) { return aus; }
    // Die Ecke der Welt, die am weitesten hinten liegt: ist sie noch davor, faellt nichts weg
    float weiteste = 0.0F;
    for (int k = 0; k < 3; ++k) { weiteste += f.normale[k] * (f.normale[k] >= 0.0F ? g.models[0].mins[k] : g.models[0].maxs[k]); }
    if (weiteste >= f.abstand) { return aus; }
    aus.blaetter.assign(g.leafs.size(), 0);
    for (std::size_t i = 0; i < g.leafs.size(); ++i) { aus.blaetter[i] = ganzDahinter(f, g.leafs[i].mins, g.leafs[i].maxs) ? 1 : 0; }
    aus.flaechen.assign(g.surfaces.size(), 0);
    for (std::size_t i = 0; i < g.surfaces.size() && i < kaesten.size(); ++i) {
        if (kaesten[i].hat) { aus.flaechen[i] = ganzDahinter(f, kaesten[i].mins, kaesten[i].maxs) ? 1 : 0; }
    }
    return aus;
}

std::vector<char> flaechenAusserSicht(const BspGeometry& g, const AreaPortale& p, int auge, int cluster, const FernStand* fern) {
    std::vector<char> verdeckt(g.surfaces.size(), 0);
    const bool mitGebiet = auge >= 0 && p.gebiete() > 0;
    // R_ClusterPVS: ausserhalb der Tabelle novis (alle Bits gesetzt)
    const std::size_t pvsAb = static_cast<std::size_t>(std::max(cluster, 0)) * static_cast<std::size_t>(std::max(g.clusterBytes, 0));
    const bool mitPvs = cluster >= 0 && cluster < g.numClusters && g.clusterBytes > 0 && pvsAb + static_cast<std::size_t>(g.clusterBytes) <= g.vis.size();
    const bool mitFern = fern != nullptr && fern->blaetter.size() == g.leafs.size() && fern->flaechen.size() == g.surfaces.size();
    if (!mitGebiet && !mitPvs && !mitFern) { return verdeckt; }
    std::vector<char> gesehen(g.surfaces.size(), 0);
    std::vector<char> inBlatt(g.surfaces.size(), 0);
    for (std::size_t bi = 0; bi < g.leafs.size(); ++bi) {
        const BspGeometry::Leaf& lf = g.leafs[bi];
        if (lf.cluster < 0 || lf.cluster >= g.numClusters || lf.firstSurface < 0 || lf.numSurfaces <= 0) { continue; }
        const bool imPvs = !mitPvs || (g.vis[pvsAb + static_cast<std::size_t>(lf.cluster >> 3)] & (1U << (lf.cluster & 7))) != 0U;
        const bool frei = imPvs && (!mitGebiet || lf.area == -1 || p.verbunden(auge, lf.area)) && !(mitFern && fern->blaetter[bi] != 0);
        const long long bis = std::min<long long>(static_cast<long long>(lf.firstSurface) + lf.numSurfaces, static_cast<long long>(g.leafSurfaces.size()));
        for (long long i = lf.firstSurface; i < bis; ++i) {
            const int s = g.leafSurfaces[static_cast<std::size_t>(i)];
            if (s < 0 || static_cast<std::size_t>(s) >= g.surfaces.size()) { continue; }
            inBlatt[static_cast<std::size_t>(s)] = 1;
            if (frei) { gesehen[static_cast<std::size_t>(s)] = 1; }
        }
    }
    for (std::size_t s = 0; s < verdeckt.size(); ++s) {
        // R_RecursiveWorldNode gibt die Flaechen der nicht markierten Blaetter gar nicht erst weiter.
        verdeckt[s] = ((inBlatt[s] != 0 && gesehen[s] == 0) || (mitFern && inBlatt[s] != 0 && fern->flaechen[s] != 0)) ? 1 : 0;
    }
    return verdeckt;
}

std::vector<PortalEntity> portalEntities(const MapData& karte, const BspGeometry& g, bool mitUndurchsichtig, bool (*spawnt)(const MapEntity&)) {
    std::vector<PortalEntity> aus;
    std::map<std::string, bool> teamDa;   // G_FindTeams: der erste einer Mannschaft ist der teammaster
    for (const MapEntity& e : karte.entities) {
        const std::string* mdl = e.find("model");
        if (mdl == nullptr || mdl->size() < 2 || (*mdl)[0] != '*' || (spawnt != nullptr && !spawnt(e))) { continue; }
        const int nummer = std::atoi(mdl->c_str() + 1);
        if (nummer <= 0 || static_cast<std::size_t>(nummer) >= g.models.size()) { continue; }
        bool sklave = false;
        const std::string* team = e.find("team");
        if (team != nullptr && !team->empty()) {
            sklave = teamDa[*team];
            teamDa[*team] = true;
        }
        PortalEntity p;
        p.modell = nummer;
        int spawnflags = 0;
        if (const std::string* sf = e.find("spawnflags")) { spawnflags = std::atoi(sf->c_str()); }
        if (e.classname == "func_wall" || e.classname == "func_usable") {
            // "START_OFF doesn't effect area portals" (g_mover.cpp, g_usable.cpp)
            if ((spawnflags & 1) != 0) { continue; }
            p.art = PortalEntity::Art::Abschaltbar;
        } else if (e.classname == "func_breakable" || e.classname == "misc_model_breakable") {
            p.art = PortalEntity::Art::Zerbrechlich;
        } else if (e.classname == "func_door" || e.classname == "func_plat" || e.classname == "func_button") {
            // Use_BinaryMover: "if ( ent->teammaster == ent || !ent->teammaster )"
            if (sklave) { continue; }
            p.art = PortalEntity::Art::Mover;
        } else {
            continue;
        }
        const BspGeometry::SubModel& m = g.models[static_cast<std::size_t>(nummer)];
        if (mitUndurchsichtig && (m.inhalt & kInhaltUndurchsichtig) == 0U) { continue; }
        float ort[3] = {0.0F, 0.0F, 0.0F};
        if (!e.origin.empty()) {
            std::istringstream is(e.origin);
            is >> ort[0] >> ort[1] >> ort[2];
        }
        float lo[3];
        float hi[3];
        for (int k = 0; k < 3; ++k) {   // SV_LinkEntity: um eine Einheit erweitert
            lo[k] = ort[k] + m.mins[k] - 1.0F;
            hi[k] = ort[k] + m.maxs[k] + 1.0F;
        }
        p.gebiete = kastenGebiete(g, lo, hi);
        if (p.gebiete.b == -1) { continue; }   // "if ( svEnt->areanum2 == -1 ) return;"
        aus.push_back(p);
    }
    return aus;
}

}  // namespace bhed
