// himmelbox.cpp - siehe bhed/himmelbox.h
#include "bhed/himmelbox.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <utility>

namespace bhed {

namespace {

constexpr int kHalbeTeilung = 4;              // HALF_SKY_SUBDIVISIONS
constexpr int kEckenGrenze = 1000;            // SHADER_MAX_VERTEXES
constexpr int kIndexGrenze = 6 * kEckenGrenze;   // SHADER_MAX_INDEXES
constexpr int kKlipEcken = 64;                // MAX_CLIP_VERTS
constexpr float kAufEbene = 0.1F;             // ON_EPSILON
constexpr float kFernWert = 524288.0F;        // MAX_WORLD_COORD bei ClearSkyBox: groesser als jede Projektion

float punkt(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// sky_clip
constexpr float kSkyKlip[6][3] = {{1, 1, 0}, {1, -1, 0}, {0, -1, 1}, {0, 1, 1}, {1, 0, 1}, {-1, 0, 1}};
// vec_to_st: s = [0]/[2], t = [1]/[2]
constexpr int kVecZuSt[6][3] = {{-2, 3, 1}, {2, 3, -1}, {1, 3, 2}, {-1, 3, -2}, {-2, -1, 3}, {-2, 1, -3}};
// st_to_vec: 1 = s, 2 = t, 3 = Box
constexpr int kStZuVec[6][3] = {{3, -1, 2}, {-3, 1, 2}, {1, 3, 2}, {-1, -3, 2}, {-2, -1, 3}, {2, -1, -3}};

float teil(const float* v, int j) { return j < 0 ? -v[-j - 1] : v[j - 1]; }

struct Kasten {
    float mins[2][6];
    float maxs[2][6];
    void leeren() {
        for (int i = 0; i < 6; ++i) {
            mins[0][i] = mins[1][i] = kFernWert;
            maxs[0][i] = maxs[1][i] = -kFernWert;
        }
    }
};

int seiteVon(const float v[3]) {
    const float a0 = std::fabs(v[0]);
    const float a1 = std::fabs(v[1]);
    const float a2 = std::fabs(v[2]);
    if (a0 > a1 && a0 > a2) { return v[0] < 0.0F ? 1 : 0; }
    if (a1 > a2 && a1 > a0) { return v[1] < 0.0F ? 3 : 2; }
    return v[2] < 0.0F ? 5 : 4;
}

// AddSkyPolygon
void addPolygon(Kasten& k, int zahl, const float* ecken) {
    float summe[3] = {0.0F, 0.0F, 0.0F};
    for (int i = 0; i < zahl; ++i) {
        for (int j = 0; j < 3; ++j) { summe[j] += ecken[i * 3 + j]; }
    }
    const int achse = seiteVon(summe);
    for (int i = 0; i < zahl; ++i) {
        const float* v = ecken + i * 3;
        const float dv = teil(v, kVecZuSt[achse][2]);
        if (dv < 0.001F) { continue; }
        const float s = teil(v, kVecZuSt[achse][0]) / dv;
        const float t = teil(v, kVecZuSt[achse][1]) / dv;
        k.mins[0][achse] = std::min(k.mins[0][achse], s);
        k.mins[1][achse] = std::min(k.mins[1][achse], t);
        k.maxs[0][achse] = std::max(k.maxs[0][achse], s);
        k.maxs[1][achse] = std::max(k.maxs[1][achse], t);
    }
}

// ClipSkyPolygon. ecken muss eine Ecke mehr fassen als zahl.
void klippe(Kasten& k, int zahl, float* ecken, int stufe) {
    if (zahl > kKlipEcken - 2) { return; }
    if (stufe == 6) {
        addPolygon(k, zahl, ecken);
        return;
    }
    float abst[kKlipEcken];
    int seiten[kKlipEcken];
    bool vorn = false;
    bool hinten = false;
    const float* n = kSkyKlip[stufe];
    int i = 0;
    for (i = 0; i < zahl; ++i) {
        const float d = punkt(ecken + i * 3, n);
        if (d > kAufEbene) {
            vorn = true;
            seiten[i] = 0;
        } else if (d < -kAufEbene) {
            hinten = true;
            seiten[i] = 1;
        } else {
            seiten[i] = 2;
        }
        abst[i] = d;
    }
    if (!vorn || !hinten) {
        klippe(k, zahl, ecken, stufe + 1);
        return;
    }
    seiten[i] = seiten[0];
    abst[i] = abst[0];
    for (int j = 0; j < 3; ++j) { ecken[i * 3 + j] = ecken[j]; }
    float neu[2][kKlipEcken * 3];
    int zahlNeu[2] = {0, 0};
    auto dazu = [&](int w, const float* p) {
        for (int j = 0; j < 3; ++j) { neu[w][zahlNeu[w] * 3 + j] = p[j]; }
        ++zahlNeu[w];
    };
    for (i = 0; i < zahl; ++i) {
        const float* v = ecken + i * 3;
        if (seiten[i] == 0 || seiten[i] == 2) { dazu(0, v); }
        if (seiten[i] == 1 || seiten[i] == 2) { dazu(1, v); }
        if (seiten[i] == 2 || seiten[i + 1] == 2 || seiten[i + 1] == seiten[i]) { continue; }
        const float d = abst[i] / (abst[i] - abst[i + 1]);
        float e[3];
        for (int j = 0; j < 3; ++j) { e[j] = v[j] + d * (v[j + 3] - v[j]); }
        dazu(0, e);
        dazu(1, e);
    }
    klippe(k, zahlNeu[0], neu[0], stufe + 1);
    klippe(k, zahlNeu[1], neu[1], stufe + 1);
}

// BoxOnPlaneSide fuer eine Ebene ohne Achsenbindung: 1 davor, 2 dahinter, 3 beides.
int kastenSeite(const float mins[3], const float maxs[3], const float n[3], float abstand) {
    float d0 = 0.0F;
    float d1 = 0.0F;
    for (int i = 0; i < 3; ++i) {
        // Im Ergebnis gleich der Tabelle aus SetPlaneSignbits: dist[0] ist die weiteste Ecke, dist[1] die naechste.
        d0 += n[i] * (n[i] < 0.0F ? mins[i] : maxs[i]);
        d1 += n[i] * (n[i] < 0.0F ? maxs[i] : mins[i]);
    }
    int seiten = 0;
    if (d0 >= abstand) { seiten = 1; }
    if (d1 < abstand) { seiten |= 2; }
    return seiten;
}

}  // namespace

bool himmelRunden(float smin, float tmin, float smax, float tmax, float& s0, float& t0, float& s1, float& t1) {
    const float h = static_cast<float>(kHalbeTeilung);
    const float a0 = std::floor(smin * h) / h;
    const float a1 = std::floor(tmin * h) / h;
    const float b0 = std::ceil(smax * h) / h;
    const float b1 = std::ceil(tmax * h) / h;
    if (a0 >= b0 || a1 >= b1) { return false; }
    const auto grenze = [&](float x) { return std::clamp(static_cast<int>(x * h), -kHalbeTeilung, kHalbeTeilung); };
    const int i0 = grenze(a0);
    const int i1 = grenze(a1);
    const int j0 = grenze(b0);
    const int j1 = grenze(b1);
    if (i0 >= j0 || i1 >= j1) { return false; }   // DrawSkySide: t < maxs[1], s <= maxs[0] - keine Zeile, kein Dreieck
    s0 = static_cast<float>(i0) / h;
    t0 = static_cast<float>(i1) / h;
    s1 = static_cast<float>(j0) / h;
    t1 = static_cast<float>(j1) / h;
    return true;
}

void himmelEcke(int seite, float s, float t, float boxGroesse, float aus[3]) {
    const float b[3] = {s * boxGroesse, t * boxGroesse, boxGroesse};
    for (int j = 0; j < 3; ++j) {
        const int k = kStZuVec[seite][j];
        aus[j] = k < 0 ? -b[-k - 1] : b[k - 1];
    }
}

bool himmelDeckt(const std::vector<HimmelRechteck>& rechtecke, const float richtung[3]) {
    const int achse = seiteVon(richtung);
    const float dv = teil(richtung, kVecZuSt[achse][2]);
    if (dv <= 0.0F) { return false; }
    const float s = teil(richtung, kVecZuSt[achse][0]) / dv;
    const float t = teil(richtung, kVecZuSt[achse][1]) / dv;
    for (const HimmelRechteck& r : rechtecke) {
        if (r.seite == achse && s >= r.s0 && s <= r.s1 && t >= r.t0 && t <= r.t1) { return true; }
    }
    return false;
}

void HimmelBox::anlegen(const BspGeometry& g, const BspMesh& netz, const std::vector<HimmelKeule>& keulen) {
    g_ = &g;
    flaechen_.clear();
    dreiecke_.clear();
    himmelBlaetter_.clear();
    blattFlaechen_.clear();
    nachBlatt_.assign(g.leafs.size() + 1U, 0);
    elternKnoten_.assign(g.nodes.size(), -1);
    elternBlatt_.assign(g.leafs.size(), -1);
    knotenMitHimmel_.assign(g.nodes.size(), 0);
    blattMitHimmel_.assign(g.leafs.size(), 0);
    knotenMarke_.assign(g.nodes.size(), 0U);
    blattMarke_.assign(g.leafs.size(), 0U);
    marke_ = 0U;

    std::vector<int> platz(g.surfaces.size(), -1);
    for (const BspMesh::Run& r : netz.runs) {
        if (r.surface >= g.surfaces.size()) { continue; }
        const BspSurface& s = g.surfaces[r.surface];
        if (s.shader < 0 || static_cast<std::size_t>(s.shader) >= keulen.size() || keulen[static_cast<std::size_t>(s.shader)] == kKeinHimmel) { continue; }
        if (static_cast<std::size_t>(r.firstIndex) + r.numIndexes > netz.indexes.size()) { continue; }
        int& p = platz[r.surface];
        if (p < 0) {
            p = static_cast<int>(flaechen_.size());
            Flaeche f;
            f.art = s.type;
            f.shader = s.shader;
            f.nebel = s.fog;
            f.ecken = s.numVerts;
            f.indizes = s.numIndexes;
            f.keule = keulen[static_cast<std::size_t>(s.shader)];
            for (int k = 0; k < 3; ++k) {
                f.normale[k] = s.ebenenNormale[k];
                f.mins[k] = s.mins[k];
                f.maxs[k] = s.maxs[k];
            }
            if (s.firstVert >= 0 && static_cast<std::size_t>(s.firstVert) < g.verts.size()) {
                f.abstand = punkt(g.verts[static_cast<std::size_t>(s.firstVert)].xyz, f.normale);   // ParseFace: plane.dist
            }
            f.ersteDreieck = static_cast<std::uint32_t>(dreiecke_.size() / 9U);
            flaechen_.push_back(f);
        }
        // Die Laeufe einer Flaeche folgen im Netz aufeinander.
        Flaeche& f = flaechen_[static_cast<std::size_t>(p)];
        for (std::uint32_t i = 0; i + 2U < r.numIndexes; i += 3U) {
            for (std::uint32_t e = 0; e < 3U; ++e) {
                const std::uint32_t q = netz.indexes[r.firstIndex + i + e];
                for (int k = 0; k < 3; ++k) { dreiecke_.push_back(q < netz.verts.size() ? netz.verts[q].xyz[k] : 0.0F); }
            }
            ++f.dreiecke;
        }
    }
    flaecheMarke_.assign(flaechen_.size(), 0U);

    // Eltern, wie R_SetParent
    for (std::size_t n = 0; n < g.nodes.size(); ++n) {
        for (const int c : g.nodes[n].children) {
            if (c >= 0 && static_cast<std::size_t>(c) < g.nodes.size()) {
                elternKnoten_[static_cast<std::size_t>(c)] = static_cast<int>(n);
            } else if (c < 0 && static_cast<std::size_t>(-c - 1) < g.leafs.size()) {
                elternBlatt_[static_cast<std::size_t>(-c - 1)] = static_cast<int>(n);
            }
        }
    }
    // Himmelsflaechen je Blatt; die Reihenfolge ist die von dleaf_t (R_RecursiveWorldNode: mark++).
    for (std::size_t b = 0; b < g.leafs.size(); ++b) {
        const BspGeometry::Leaf& lf = g.leafs[b];
        nachBlatt_[b] = static_cast<int>(blattFlaechen_.size());
        if (lf.firstSurface < 0 || lf.numSurfaces <= 0) { continue; }
        const long long bis = std::min<long long>(static_cast<long long>(lf.firstSurface) + lf.numSurfaces, static_cast<long long>(g.leafSurfaces.size()));
        for (long long i = lf.firstSurface; i < bis; ++i) {
            const int s = g.leafSurfaces[static_cast<std::size_t>(i)];
            if (s < 0 || static_cast<std::size_t>(s) >= platz.size() || platz[static_cast<std::size_t>(s)] < 0) { continue; }
            blattFlaechen_.push_back(platz[static_cast<std::size_t>(s)]);
        }
        if (static_cast<int>(blattFlaechen_.size()) != nachBlatt_[b]) {
            himmelBlaetter_.push_back(static_cast<int>(b));
            blattMitHimmel_[b] = 1;
            for (int e = elternBlatt_[b]; e >= 0 && knotenMitHimmel_[static_cast<std::size_t>(e)] == 0; e = elternKnoten_[static_cast<std::size_t>(e)]) {
                knotenMitHimmel_[static_cast<std::size_t>(e)] = 1;
            }
        }
    }
    nachBlatt_[g.leafs.size()] = static_cast<int>(blattFlaechen_.size());
}

bool HimmelBox::flaecheKeulen(const Flaeche& f) const {
    if (f.art == BspSurface::Type::Planar) {
        if (f.keule == kHimmelBeide) { return false; }
        const float d = punkt(auge_, f.normale);
        return f.keule == kHimmelVorn ? d < f.abstand - 8.0F : d > f.abstand + 8.0F;
    }
    if (f.art == BspSurface::Type::Patch || f.art == BspSurface::Type::TriangleSoup) {
        // R_CullLocalBox gegen die vier Seitenebenen; R_CullGrid fragt erst die Kugel um den Kasten, dann den Kasten.
        for (int e = 0; e < 4; ++e) {
            bool vorn = false;
            for (int k = 0; k < 8 && !vorn; ++k) {
                const float p[3] = {(k & 1) != 0 ? f.maxs[0] : f.mins[0], (k & 2) != 0 ? f.maxs[1] : f.mins[1], (k & 4) != 0 ? f.maxs[2] : f.mins[2]};
                vorn = punkt(p, frustum_[e].normale) > frustum_[e].abstand;
            }
            if (!vorn) { return true; }
        }
        if (f.art == BspSurface::Type::Patch) {
            float mitte[3];
            float tmp[3];
            for (int k = 0; k < 3; ++k) {
                mitte[k] = 0.5F * (f.mins[k] + f.maxs[k]);
                tmp[k] = f.mins[k] - mitte[k];
            }
            const float radius = std::sqrt(punkt(tmp, tmp));
            for (int e = 0; e < 4; ++e) {
                if (punkt(mitte, frustum_[e].normale) - frustum_[e].abstand < -radius) { return true; }
            }
        }
    }
    return false;
}

void HimmelBox::blattHinzu(int blatt) {
    for (int i = nachBlatt_[static_cast<std::size_t>(blatt)]; i < nachBlatt_[static_cast<std::size_t>(blatt) + 1U]; ++i) {
        const int f = blattFlaechen_[static_cast<std::size_t>(i)];
        if (flaecheMarke_[static_cast<std::size_t>(f)] == marke_) { continue; }
        flaecheMarke_[static_cast<std::size_t>(f)] = marke_;
        if (!flaecheKeulen(flaechen_[static_cast<std::size_t>(f)])) { imBild_.push_back(f); }
    }
}

void HimmelBox::geheDurch(int knoten, int ebenenBits) {
    // R_RecursiveWorldNode. Aeste ohne Himmel fallen gleich weg, sie brachten nichts dazu.
    while (true) {
        const bool blatt = knoten < 0;
        const auto index = static_cast<std::size_t>(blatt ? -knoten - 1 : knoten);
        if (blatt ? (index >= g_->leafs.size() || blattMitHimmel_[index] == 0) : (index >= g_->nodes.size() || knotenMitHimmel_[index] == 0)) { return; }
        if (!alleSichtbar_ && (blatt ? blattMarke_[index] : knotenMarke_[index]) != marke_) { return; }
        const float* mins = blatt ? g_->leafs[index].mins : g_->nodes[index].mins;
        const float* maxs = blatt ? g_->leafs[index].maxs : g_->nodes[index].maxs;
        for (int e = 0; e < 5; ++e) {
            if ((ebenenBits & (1 << e)) == 0) { continue; }
            const int r = kastenSeite(mins, maxs, frustum_[e].normale, frustum_[e].abstand);
            if (r == 2) { return; }
            if (r == 1) { ebenenBits &= ~(1 << e); }
        }
        if (blatt) {
            blattHinzu(static_cast<int>(index));
            return;
        }
        geheDurch(g_->nodes[index].children[0], ebenenBits);
        knoten = g_->nodes[index].children[1];
    }
}

void HimmelBox::rechne(const HimmelSicht& sicht, std::vector<HimmelRechteck>& aus) {
    aus.clear();
    imBild_.clear();
    if (g_ == nullptr || flaechen_.empty()) { return; }
    ++marke_;
    for (int k = 0; k < 3; ++k) { auge_[k] = sicht.auge[k]; }

    // R_SetupFrustum
    const auto ebene = [&](Ebene& e, const float* quer, float grad, float vz) {
        const float w = grad / 180.0F * 3.14159265358979323846F * 0.5F;
        for (int k = 0; k < 3; ++k) { e.normale[k] = sicht.vorn[k] * std::sin(w) + vz * std::cos(w) * quer[k]; }
        e.abstand = punkt(sicht.auge, e.normale);
    };
    ebene(frustum_[0], sicht.links, sicht.fovX, 1.0F);
    ebene(frustum_[1], sicht.links, sicht.fovX, -1.0F);
    ebene(frustum_[2], sicht.hoch, sicht.fovY, 1.0F);
    ebene(frustum_[3], sicht.hoch, sicht.fovY, -1.0F);
    for (int k = 0; k < 3; ++k) { frustum_[4].normale[k] = -sicht.vorn[k]; }
    frustum_[4].abstand = punkt(sicht.auge, frustum_[4].normale) - sicht.distanceCull * 1.02F;

    // R_MarkLeaves
    const int blattAuge = g_->leafAt(sicht.pvsOrt);
    const int cluster = blattAuge >= 0 && static_cast<std::size_t>(blattAuge) < g_->leafs.size() ? g_->leafs[static_cast<std::size_t>(blattAuge)].cluster : -1;
    alleSichtbar_ = cluster < 0;
    if (!alleSichtbar_) {
        for (const int b : himmelBlaetter_) {
            const BspGeometry::Leaf& lf = g_->leafs[static_cast<std::size_t>(b)];
            if (lf.cluster < 0 || lf.cluster >= g_->numClusters) { continue; }
            // R_ClusterPVS: Ohne Tabelle (oder ausserhalb) sieht man alles.
            if (!g_->vis.empty() && cluster < g_->numClusters) {
                const std::size_t at = static_cast<std::size_t>(cluster) * static_cast<std::size_t>(g_->clusterBytes) + static_cast<std::size_t>(lf.cluster >> 3);
                if (at >= g_->vis.size() || (g_->vis[at] & (1U << (lf.cluster & 7))) == 0U) { continue; }
            }
            if (sicht.portale != nullptr && sicht.gebiet >= 0 && lf.area >= 0 && !sicht.portale->verbunden(sicht.gebiet, lf.area)) { continue; }
            blattMarke_[static_cast<std::size_t>(b)] = marke_;
            for (int e = elternBlatt_[static_cast<std::size_t>(b)]; e >= 0 && knotenMarke_[static_cast<std::size_t>(e)] != marke_; e = elternKnoten_[static_cast<std::size_t>(e)]) {
                knotenMarke_[static_cast<std::size_t>(e)] = marke_;
            }
        }
    }
    if (!g_->nodes.empty()) {
        geheDurch(0, 31);
    } else if (!g_->leafs.empty()) {
        geheDurch(-1, 31);
    }

    // R_RadixSort ist stabil und ordnet nach Shader, dann Nebel: Je Paar bleibt die Reihenfolge von oben.
    std::map<std::pair<int, int>, std::vector<int>> gruppen;
    for (const int f : imBild_) {
        const Flaeche& x = flaechen_[static_cast<std::size_t>(f)];
        gruppen[{x.shader, x.nebel}].push_back(f);
    }
    Kasten k;
    const auto fertig = [&](int shader) {
        for (int seite = 0; seite < 5; ++seite) {   // i == 5: "still don't want to draw the bottom"
            HimmelRechteck r;
            r.shader = shader;
            r.seite = seite;
            if (himmelRunden(k.mins[0][seite], k.mins[1][seite], k.maxs[0][seite], k.maxs[1][seite], r.s0, r.t0, r.s1, r.t1)) { aus.push_back(r); }
        }
    };
    for (const auto& [schluessel, liste] : gruppen) {
        k.leeren();
        int ecken = 0;
        int indizes = 0;
        for (const int fi : liste) {
            const Flaeche& f = flaechen_[static_cast<std::size_t>(fi)];
            // RB_CHECKOVERFLOW: Wird der Puffer voll, zeichnet die Engine schon jetzt und beginnt von vorn.
            if (ecken > 0 && (ecken + f.ecken >= kEckenGrenze || indizes + f.indizes >= kIndexGrenze)) {
                fertig(schluessel.first);
                k.leeren();
                ecken = 0;
                indizes = 0;
            }
            ecken += f.ecken;
            indizes += f.indizes;
            // RB_ClipSkyPolygons
            for (std::uint32_t d = 0; d < f.dreiecke; ++d) {
                const float* q = dreiecke_.data() + static_cast<std::size_t>(f.ersteDreieck + d) * 9U;
                float p[5 * 3];
                for (int e = 0; e < 3; ++e) {
                    for (int a = 0; a < 3; ++a) { p[e * 3 + a] = q[e * 3 + a] - sicht.auge[a]; }
                }
                klippe(k, 3, p, 0);
            }
        }
        fertig(schluessel.first);
    }
}

}  // namespace bhed
