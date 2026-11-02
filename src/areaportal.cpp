// areaportal.cpp - siehe bhed/areaportal.h
#include "bhed/areaportal.h"

#include <algorithm>
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

std::vector<char> flaechenHinterPortalen(const BspGeometry& g, const AreaPortale& p, int auge) {
    std::vector<char> verdeckt(g.surfaces.size(), 0);
    if (auge < 0 || p.gebiete() == 0) { return verdeckt; }
    std::vector<char> gesehen(g.surfaces.size(), 0);
    std::vector<char> inBlatt(g.surfaces.size(), 0);
    for (const BspGeometry::Leaf& lf : g.leafs) {
        if (lf.cluster < 0 || lf.firstSurface < 0 || lf.numSurfaces <= 0) { continue; }
        const bool frei = lf.area == -1 || p.verbunden(auge, lf.area);
        const long long bis = std::min<long long>(static_cast<long long>(lf.firstSurface) + lf.numSurfaces, static_cast<long long>(g.leafSurfaces.size()));
        for (long long i = lf.firstSurface; i < bis; ++i) {
            const int s = g.leafSurfaces[static_cast<std::size_t>(i)];
            if (s < 0 || static_cast<std::size_t>(s) >= g.surfaces.size()) { continue; }
            inBlatt[static_cast<std::size_t>(s)] = 1;
            if (frei) { gesehen[static_cast<std::size_t>(s)] = 1; }
        }
    }
    for (std::size_t s = 0; s < verdeckt.size(); ++s) {
        // Auch der Himmel: R_RecursiveWorldNode gibt seine Flaechen hinter einem geschlossenen Portal gar nicht erst weiter.
        verdeckt[s] = (inBlatt[s] != 0 && gesehen[s] == 0) ? 1 : 0;
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
