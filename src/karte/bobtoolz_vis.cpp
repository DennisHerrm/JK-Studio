// bobtoolz_vis.cpp - Karten-Editor, Stufe 4: der Vis Viewer von bobToolz
// (welche Flaechen ein Blatt der .bsp sieht).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/bobtoolz: bsploader.cpp
// (LoadBSPFile: IBSP und RBSP), visfind.cpp (bsp_leafnumfororigin,
// bsp_planesForLeaf, BuildLeafWindings, PutPointsOnCurve, AddSurface,
// AddLeafSurfaces, TraceCluster, SetupVisView, BuildTrace), DVisDrawer.cpp
// (ui_leaf_add, ui_show: Tabelle nach "surfs visible" absteigend,
// ui_leaf_show).
//
//   BobToolz plugin for GtkRadiant
//   Copyright (C) 2001 Gordon Biggans
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2: This program is free software; you can redistribute
//   it and/or modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; version 2 of the License. This
//   program is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
//   details (LICENSE).
#include "bobtoolz_d.h"

#include "bhed/karte/bspoeffnen.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::karte::bobtoolz {

using namespace d;

namespace {

constexpr int MST_BAD = 0;
constexpr int MST_PLANAR = 1;
constexpr int MST_PATCH = 2;
constexpr int MST_TRIANGLE_SOUP = 3;

struct dplane_t {
    float normal[3];
    float dist;
};
struct dnode_t {
    int planeNum;
    int children[2];
    int mins[3];
    int maxs[3];
};
struct dleaf_t {
    int cluster;
    int area;
    int mins[3];
    int maxs[3];
    int firstLeafSurface;
    int numLeafSurfaces;
    int firstLeafBrush;
    int numLeafBrushes;
};
struct dsurface_t {   // die Felder, die visfind braucht (aus dsurface_t bzw. rbspDrawSurface_t)
    int shaderNum;
    int surfaceType;
    int firstVert;
    int numVerts;
    int firstIndex;
    int numIndexes;
    int patchWidth;
    int patchHeight;
};
struct vis_header {
    int portalclusters;
    int leafbytes;
};

struct BspDaten {   // die globalen Felder von bsploader.cpp
    std::vector<dplane_t> dplanes;
    std::vector<dnode_t> dnodes;
    std::vector<dleaf_t> dleafs;
    std::vector<int> dleafsurfaces;
    std::vector<std::array<float, 3>> drawVerts;   // nur xyz
    std::vector<int> drawVertsIndices;
    std::vector<dsurface_t> drawSurfaces;
    std::vector<unsigned char> visBytes;
};

int leseInt(const std::string& d, std::size_t o) {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(d[o + i])) << (8 * i); }
    return static_cast<int>(v);
}
float leseFloat(const std::string& d, std::size_t o) {
    const int i = leseInt(d, o);
    float f = 0;
    std::memcpy(&f, &i, 4);
    return f;
}

// LoadBSPFile (bsploader.cpp:208-269); "" = in Ordnung, sonst die Meldung des Kastens
std::string bspLaden(const std::string& pfad, BspDaten& b) {
    std::ifstream f(std::filesystem::path(std::u8string(pfad.begin(), pfad.end())), std::ios::binary);
    if (!f) { return "BSP file not found"; }
    std::ostringstream s;
    s << f.rdbuf();
    const std::string daten = s.str();
    BspDatei bsp;
    if (!bspLesen(daten, bsp)) { return "Cant find a valid IBSP/RBSP/FBSP file"; }
    const bool rbsp = bsp.ident == "RBSP";
    if (!((bsp.ident == "IBSP" && (bsp.version == 46 || bsp.version == 47)) || (rbsp && bsp.version == 1))) { return "File is incorrect version"; }
    const std::string& planes = bsp.lumps[2];
    for (std::size_t o = 0; o + 16 <= planes.size(); o += 16) {
        b.dplanes.push_back({{leseFloat(planes, o), leseFloat(planes, o + 4), leseFloat(planes, o + 8)}, leseFloat(planes, o + 12)});
    }
    const std::string& nodes = bsp.lumps[3];
    for (std::size_t o = 0; o + 36 <= nodes.size(); o += 36) {
        dnode_t n{};
        n.planeNum = leseInt(nodes, o);
        n.children[0] = leseInt(nodes, o + 4);
        n.children[1] = leseInt(nodes, o + 8);
        b.dnodes.push_back(n);
    }
    const std::string& leafs = bsp.lumps[4];
    for (std::size_t o = 0; o + 48 <= leafs.size(); o += 48) {
        dleaf_t l{};
        l.cluster = leseInt(leafs, o);
        l.area = leseInt(leafs, o + 4);
        l.firstLeafSurface = leseInt(leafs, o + 32);
        l.numLeafSurfaces = leseInt(leafs, o + 36);
        b.dleafs.push_back(l);
    }
    const std::string& ls = bsp.lumps[5];
    for (std::size_t o = 0; o + 4 <= ls.size(); o += 4) { b.dleafsurfaces.push_back(leseInt(ls, o)); }
    const std::string& verts = bsp.lumps[10];
    const std::size_t vGroesse = rbsp ? 80 : 44;   // rbspDrawVert_t / qdrawVert_t
    for (std::size_t o = 0; o + vGroesse <= verts.size(); o += vGroesse) {
        b.drawVerts.push_back({leseFloat(verts, o), leseFloat(verts, o + 4), leseFloat(verts, o + 8)});
    }
    const std::string& idx = bsp.lumps[11];
    for (std::size_t o = 0; o + 4 <= idx.size(); o += 4) { b.drawVertsIndices.push_back(leseInt(idx, o)); }
    const std::string& surfs = bsp.lumps[13];
    const std::size_t sGroesse = rbsp ? 148 : 104;   // rbspDrawSurface_t / dsurface_t
    for (std::size_t o = 0; o + sGroesse <= surfs.size(); o += sGroesse) {
        dsurface_t ds{};
        ds.shaderNum = leseInt(surfs, o);
        ds.surfaceType = leseInt(surfs, o + 8);
        ds.firstVert = leseInt(surfs, o + 12);
        ds.numVerts = leseInt(surfs, o + 16);
        ds.firstIndex = leseInt(surfs, o + 20);
        ds.numIndexes = leseInt(surfs, o + 24);
        ds.patchWidth = leseInt(surfs, o + sGroesse - 8);
        ds.patchHeight = leseInt(surfs, o + sGroesse - 4);
        b.drawSurfaces.push_back(ds);
    }
    const std::string& vis = bsp.lumps[16];
    b.visBytes.assign(vis.begin(), vis.end());
    return {};
}

int bsp_leafnumfororigin(const BspDaten& b, const vec_t* origin) {   // visfind.cpp:29-57
    if (b.dnodes.empty()) { return 0; }
    const dnode_t* node = b.dnodes.data();
    for (;;) {
        const dplane_t& plane = b.dplanes[static_cast<std::size_t>(node->planeNum)];
        const float d = DotProduct(origin, plane.normal) - plane.dist;
        if (d >= 0) {
            if (node->children[0] < 0) { return -(node->children[0] + 1); }
            node = &b.dnodes[static_cast<std::size_t>(node->children[0])];
        } else if (node->children[1] < 0) {
            return -(node->children[1] + 1);
        } else {
            node = &b.dnodes[static_cast<std::size_t>(node->children[1])];
        }
    }
}

std::vector<const dplane_t*> bsp_planesForLeaf(const BspDaten& b, int leafnum) {   // visfind.cpp:59-80
    std::vector<const dplane_t*> planes;
    int findnum = -(leafnum + 1);
    const auto cmp = [&](const dnode_t& node) {
        if (findnum == node.children[0]) {
            planes.push_back(&b.dplanes[static_cast<std::size_t>(node.planeNum)]);
            return true;
        }
        if (findnum == node.children[1]) {
            planes.push_back(&b.dplanes[static_cast<std::size_t>(node.planeNum ^ 1)]);
            return true;
        }
        return false;
    };
    auto node = std::find_if(b.dnodes.begin(), b.dnodes.end(), cmp);
    while (node != b.dnodes.end()) {
        findnum = static_cast<int>(node - b.dnodes.begin());
        node = std::find_if(b.dnodes.begin(), b.dnodes.end(), cmp);
    }
    return planes;
}

std::vector<std::vector<Vektor3f>> BuildLeafWindings(const BspDaten& b, int leafnum) {   // visfind.cpp:82-110
    std::vector<std::vector<Vektor3f>> windings;
    const std::vector<const dplane_t*> planes = bsp_planesForLeaf(b, leafnum);
    DPlane dplane;
    for (const dplane_t* plane : planes) {
        VectorCopy(plane->normal, dplane.normal);
        dplane._d = plane->dist;
        DWinding winding = dplane.BaseWindingForPlane();
        for (const dplane_t* cplane : planes) {
            if (winding.numpoints <= 2) { break; }
            if (cplane != plane) {
                VectorCopy(cplane->normal, dplane.normal);
                dplane._d = cplane->dist;
                (void)winding.ChopWindingInPlace(&dplane, .1F);
            }
        }
        if (winding.numpoints > 2) {
            std::vector<Vektor3f> w;
            for (int i = 0; i < winding.numpoints; ++i) {
                const auto& p = winding.p[static_cast<std::size_t>(i)];
                w.emplace_back(p[0], p[1], p[2]);
            }
            windings.push_back(std::move(w));
        }
    }
    return windings;
}

bool bit_is_enabled(const unsigned char* bytes, int bit_index) { return (bytes[bit_index >> 3] & (1 << (bit_index & 7))) != 0; }

void PutPointsOnCurve(std::vector<Vektor3f>& ctrl, int width, int height) {   // visfind.cpp:180-200
    const auto lerp3 = [](const Vektor3f& prev, Vektor3f& mid, const Vektor3f& next) {
        for (std::size_t c = 0; c < 3; ++c) { mid[c] = static_cast<float>((prev[c] + mid[c] * 2 + next[c]) * .25); }
    };
    for (int i = 0; i < width; ++i) {
        for (int j = 1; j < height; j += 2) {
            lerp3(ctrl[static_cast<std::size_t>((j - 1) * width + i)], ctrl[static_cast<std::size_t>((j + 0) * width + i)],
                  ctrl[static_cast<std::size_t>((j + 1) * width + i)]);
        }
    }
    for (int j = 0; j < height; ++j) {
        for (int i = 1; i < width; i += 2) {
            lerp3(ctrl[static_cast<std::size_t>(j * width + i - 1)], ctrl[static_cast<std::size_t>(j * width + i + 0)],
                  ctrl[static_cast<std::size_t>(j * width + i + 1)]);
        }
    }
}

const Vektor3f debugColorMain(0, 255, 0);   // visfind.cpp:203 (so bei NRC: 255, nicht 1)
const Vektor3f debugColors[16] = {{1, 0, 0},       {0.75F, 0.5F, 0.5F}, {1, 1, 0},          {0.75F, 0.75F, 0.5F}, {0, 1, 1},       {0.5F, 0.75F, 0.75F},
                                  {0, 0, 1},       {0.5F, 0.5F, 0.75F}, {1, 0, 1},          {0.75F, 0.5F, 0.75F}, {0.5F, 0.75F, 0.5F}, {0.75F, 0, 0.5F},
                                  {0.5F, 0, 0.75F}, {0, 0.5F, 0.75F},   {0.75F, 0.5F, 0}, {1, 0, 0.5F}};

void AddSurface(const BspDaten& b, const dsurface_t& surf, const Vektor3f& clr, std::vector<VisFlaeche>& pointlist) {   // visfind.cpp:233-268
    if (surf.surfaceType == MST_PLANAR || surf.surfaceType == MST_TRIANGLE_SOUP) {
        VisFlaeche& meta = pointlist.emplace_back();
        for (int l = 0; l < surf.numIndexes; ++l) { meta.indizes.push_back(static_cast<unsigned>(b.drawVertsIndices[static_cast<std::size_t>(surf.firstIndex + l)])); }
        for (int l = 0; l < surf.numVerts; ++l) {
            const auto& v = b.drawVerts[static_cast<std::size_t>(surf.firstVert + l)];
            meta.ecken.emplace_back(v[0], v[1], v[2]);
        }
        meta.farbe = clr;
    } else if (surf.surfaceType == MST_PATCH) {
        VisFlaeche& meta = pointlist.emplace_back();
        for (int l = 0; l < surf.numVerts; ++l) {
            const auto& v = b.drawVerts[static_cast<std::size_t>(surf.firstVert + l)];
            meta.ecken.emplace_back(v[0], v[1], v[2]);
        }
        meta.farbe = clr;
        PutPointsOnCurve(meta.ecken, surf.patchWidth, surf.patchHeight);
        for (int i = 0; i < surf.patchWidth - 1; ++i) {
            for (int j = 0; j < surf.patchHeight - 1; ++j) {
                const auto a = static_cast<unsigned>((j + 0) * surf.patchWidth + i);
                const auto c = static_cast<unsigned>((j + 1) * surf.patchWidth + i + 1);
                meta.indizes.insert(meta.indizes.end(),
                                    {a, static_cast<unsigned>((j + 1) * surf.patchWidth + i), c, a, c, static_cast<unsigned>((j + 0) * surf.patchWidth + i + 1)});
            }
        }
    }
}

void AddLeafSurfaces(const BspDaten& b, const dleaf_t& leaf, const Vektor3f& clr, bool colorPerSurf, std::vector<bool>& repeatlist,
                     std::vector<VisFlaeche>& pointlist) {   // visfind.cpp:270-279
    for (int i = 0; i < leaf.numLeafSurfaces; ++i) {
        const int leafsurf = b.dleafsurfaces[static_cast<std::size_t>(leaf.firstLeafSurface + i)];
        if (!repeatlist[static_cast<std::size_t>(leafsurf)]) {
            repeatlist[static_cast<std::size_t>(leafsurf)] = true;
            AddSurface(b, b.drawSurfaces[static_cast<std::size_t>(leafsurf)], colorPerSurf ? debugColors[leafsurf & 15] : clr, pointlist);
        }
    }
}

vis_header kopf(const BspDaten& b) { return {leseInt(std::string(b.visBytes.begin(), b.visBytes.begin() + 8), 0), leseInt(std::string(b.visBytes.begin(), b.visBytes.begin() + 8), 4)}; }

std::vector<VisFlaeche> doTraceCluster(const BspDaten& b, int leafnum, bool colorPerSurf) {   // TraceCluster::doTraceCluster
    std::vector<VisFlaeche> pointlist;
    std::vector<bool> repeatlist(b.drawSurfaces.size(), false);
    const vis_header vheader = kopf(b);
    const dleaf_t& leaf = b.dleafs[static_cast<std::size_t>(leafnum)];
    const unsigned char* vis = b.visBytes.data() + 8 + (leaf.cluster * vheader.leafbytes);
    AddLeafSurfaces(b, leaf, debugColorMain, false, repeatlist, pointlist);
    for (std::size_t c = 0; c < b.dleafs.size(); ++c) {
        const dleaf_t& cleaf = b.dleafs[c];
        if (cleaf.cluster >= 0 && cleaf.cluster < vheader.portalclusters && leaf.area == cleaf.area && bit_is_enabled(vis, cleaf.cluster)) {
            AddLeafSurfaces(b, cleaf, debugColors[c % 15], colorPerSurf, repeatlist, pointlist);
        }
    }
    return pointlist;
}

std::vector<VisBlatt> traverseLeafs(const BspDaten& b) {   // TraceCluster::traverseLeafs
    std::vector<VisBlatt> liste;
    const vis_header vheader = kopf(b);
    std::vector<bool> repeatlist(b.drawSurfaces.size(), false);
    for (std::size_t li = 0; li < b.dleafs.size(); ++li) {
        const dleaf_t& leaf = b.dleafs[li];
        if (leaf.cluster < 0 || leaf.cluster >= vheader.portalclusters) { continue; }
        int nsurfs = 0;
        int nleafs = 0;
        std::vector<int> shadernums;
        const auto countLeafStuff = [&](const dleaf_t& l) {
            for (int i = 0; i < l.numLeafSurfaces; ++i) {
                const int leafsurf = b.dleafsurfaces[static_cast<std::size_t>(l.firstLeafSurface + i)];
                if (!repeatlist[static_cast<std::size_t>(leafsurf)]) {
                    repeatlist[static_cast<std::size_t>(leafsurf)] = true;
                    const dsurface_t& surf = b.drawSurfaces[static_cast<std::size_t>(leafsurf)];
                    if (surf.surfaceType != MST_BAD) { ++nsurfs; }
                    if (std::find(shadernums.begin(), shadernums.end(), surf.shaderNum) == shadernums.end()) { shadernums.push_back(surf.shaderNum); }
                }
            }
        };
        std::fill(repeatlist.begin(), repeatlist.end(), false);
        const unsigned char* vis = b.visBytes.data() + 8 + (leaf.cluster * vheader.leafbytes);
        countLeafStuff(leaf);
        for (const dleaf_t& cleaf : b.dleafs) {
            if (cleaf.cluster >= 0 && cleaf.cluster < vheader.portalclusters && leaf.area == cleaf.area && bit_is_enabled(vis, cleaf.cluster)) {
                countLeafStuff(cleaf);
                ++nleafs;
            }
        }
        liste.push_back({static_cast<int>(li), nleafs, nsurfs, static_cast<int>(shadernums.size())});
    }
    // ui_show: setSortingEnabled( true ) mit dem Sortierzeichen auf Spalte 2 absteigend
    std::stable_sort(liste.begin(), liste.end(), [](const VisBlatt& a, const VisBlatt& c) { return a.flaechen > c.flaechen; });
    return liste;
}

}  // namespace

Meldungen visAnalyse(const std::string& bspPfad, const Vektor3f& ursprung, VisAnsicht& v) {   // SetupVisView (visfind.cpp:381-425)
    Meldungen m;
    if (!v.offen) {
        auto b = std::make_shared<BspDaten>();
        const std::string fehler = bspLaden(bspPfad, *b);
        if (!fehler.empty()) {
            m.push_back({2, fehler});   // DoMessageBox( ..., "Error" )
            return m;
        }
        if (b->visBytes.size() <= sizeof(vis_header)) {
            m.push_back({2, "bobToolz VisAnalyse: Bsp has no visibility data!\n"});
            int maxcluster = -1;
            for (const dleaf_t& l : b->dleafs) { maxcluster = std::max(maxcluster, l.cluster); }
            if (maxcluster < 0) { return m; }
            m.push_back({2, "bobToolz VisAnalyse: Setting up fake visibility data!\n"});
            vis_header vheader{};
            vheader.portalclusters = maxcluster + 1;
            vheader.leafbytes = ((vheader.portalclusters + 63) & ~63) >> 3;
            b->visBytes.assign(sizeof(vis_header) + static_cast<std::size_t>(vheader.portalclusters * vheader.leafbytes), 0xFF);
            std::memcpy(b->visBytes.data(), &vheader, sizeof(vheader));
        }
        v = VisAnsicht{};
        v.bsp = bspPfad;
        v.daten = b;
        v.liste = traverseLeafs(*b);
        v.offen = true;
    }
    const auto& b = *static_cast<BspDaten*>(v.daten.get());
    vec3_t o = {ursprung[0], ursprung[1], ursprung[2]};
    const int leafnum = bsp_leafnumfororigin(b, o);
    if (leafnum < 0 || leafnum >= static_cast<int>(b.dleafs.size()) || b.dleafs[static_cast<std::size_t>(leafnum)].cluster == -1) {
        v.flaechen.clear();   // ClearPoints
        v.wicklungen.clear();
        v.gewaehlt = -1;
        m.push_back({2, "bobToolz VisAnalyse: Point of interest is in the void!\n"});
        return m;
    }
    visBlattWaehlen(v, leafnum);   // ui_leaf_show
    return m;
}

void visBlattWaehlen(VisAnsicht& v, int blatt) {   // currentCellChanged: SetList( BuildTrace ), BuildLeafWindings
    if (!v.daten) { return; }
    const auto& b = *static_cast<BspDaten*>(v.daten.get());
    if (blatt < 0 || blatt >= static_cast<int>(b.dleafs.size())) { return; }
    const bool inListe = std::any_of(v.liste.begin(), v.liste.end(), [blatt](const VisBlatt& x) { return x.blatt == blatt; });
    if (!inListe) { return; }   // ui_leaf_show findet keine Zeile
    v.gewaehlt = blatt;
    v.flaechen = doTraceCluster(b, blatt, v.farbeJeFlaeche);
    v.wicklungen = BuildLeafWindings(b, blatt);
}

}  // namespace bhed::karte::bobtoolz
