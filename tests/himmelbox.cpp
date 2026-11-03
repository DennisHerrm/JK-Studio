// rc608 Teil 8: Die Rechtecke der Himmelsbox gegen tr_sky.cpp, tr_world.cpp und tr_surface.cpp, von Hand gerechnet.
#include "bhed/areaportal.h"
#include "bhed/bspgeo.h"
#include "bhed/himmelbox.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace {

using namespace bhed;

int g_fehler = 0;

void erwarte(const char* name, bool ok) {
    std::printf("  %s %s\n", ok ? "ok  " : "FEHL", name);
    if (!ok) { ++g_fehler; }
}

bool nah(float a, float b) { return std::fabs(a - b) < 1e-4F; }

// Eine Ebene x = 0 teilt die Welt: Blatt 0 (x > 0, Gebiet 0, Cluster 0), Blatt 1 (x < 0, Gebiet 1, Cluster 1).
BspGeometry grundWelt() {
    BspGeometry g;
    BspGeometry::Plane p;
    p.normal[0] = 1.0F;
    g.planes.push_back(p);
    BspGeometry::Node n;
    n.plane = 0;
    n.children[0] = -1;
    n.children[1] = -2;
    for (int k = 0; k < 3; ++k) {
        n.mins[k] = -60000.0F;
        n.maxs[k] = 60000.0F;
    }
    g.nodes.push_back(n);
    BspGeometry::Leaf a;
    a.cluster = 0;
    a.area = 0;
    BspGeometry::Leaf b;
    b.cluster = 1;
    b.area = 1;
    for (int k = 0; k < 3; ++k) {
        a.mins[k] = b.mins[k] = -60000.0F;
        a.maxs[k] = b.maxs[k] = 60000.0F;
    }
    a.mins[0] = 0.0F;
    b.maxs[0] = 0.0F;
    g.leafs = {a, b};
    g.numClusters = 2;
    g.clusterBytes = 1;
    g.vis = {0x03, 0x03};
    BspShader s;
    s.name = "himmel";
    g.shaders.push_back(s);
    return g;
}

// Eine ebene Flaeche aus vier Ecken im Blatt `blatt`.
void flaeche(BspGeometry& g, BspMesh& m, int blatt, const float e[4][3], const float normale[3], int shader = 0) {
    BspSurface s;
    s.type = BspSurface::Type::Planar;
    s.shader = shader;
    s.firstVert = static_cast<int>(g.verts.size());
    s.numVerts = 4;
    s.numIndexes = 6;
    for (int k = 0; k < 3; ++k) { s.ebenenNormale[k] = normale[k]; }
    for (int i = 0; i < 4; ++i) {
        BspVertex v;
        for (int k = 0; k < 3; ++k) { v.xyz[k] = e[i][k]; }
        g.verts.push_back(v);
        m.verts.push_back(v);
    }
    const auto surf = static_cast<std::uint32_t>(g.surfaces.size());
    g.surfaces.push_back(s);
    BspMesh::Run r;
    r.firstIndex = static_cast<std::uint32_t>(m.indexes.size());
    r.numIndexes = 6;
    r.surface = surf;
    m.runs.push_back(r);
    const auto b = static_cast<std::uint32_t>(m.verts.size()) - 4U;
    for (const std::uint32_t i : {b, b + 1U, b + 2U, b, b + 2U, b + 3U}) { m.indexes.push_back(i); }
    // Die Blattlisten liegen je Blatt am Stueck, hier reicht das Anhaengen.
    auto& lf = g.leafs[static_cast<std::size_t>(blatt)];
    if (lf.numSurfaces == 0) { lf.firstSurface = static_cast<int>(g.leafSurfaces.size()); }
    g.leafSurfaces.push_back(static_cast<int>(surf));
    ++lf.numSurfaces;
}

// Ein Rechteck senkrecht zu +x im Abstand 1000 um (y, z), Normale zum Auge im Ursprung.
void vorDemAuge(BspGeometry& g, BspMesh& m, int blatt, float y0, float y1, float z0, float z1, float x = 1000.0F) {
    const float e[4][3] = {{x, y0, z0}, {x, y1, z0}, {x, y1, z1}, {x, y0, z1}};
    const float n[3] = {x > 0.0F ? -1.0F : 1.0F, 0.0F, 0.0F};
    flaeche(g, m, blatt, e, n);
}

HimmelSicht blickNachX(float aufX = 1.0F) {
    HimmelSicht s;
    s.auge[0] = 1.0F;   // Blatt 0
    s.pvsOrt[0] = 1.0F;
    s.vorn[0] = aufX;
    s.links[1] = aufX;
    s.hoch[2] = 1.0F;
    s.fovX = 80.0F;
    s.fovY = 64.3727F;
    return s;
}

void rechteck() {
    BspGeometry g = grundWelt();
    BspMesh m;
    vorDemAuge(g, m, 0, -99.0F, 99.0F, -99.0F, 99.0F, 1001.0F);
    HimmelBox b;
    b.anlegen(g, m, {kHimmelVorn});
    std::vector<HimmelRechteck> r;
    b.rechne(blickNachX(), r);
    // AddSkyPolygon: Seite 0, s = -y / x, t = z / x, also +-0,099. floor/ceil auf ein Viertel (HALF_SKY_SUBDIVISIONS).
    erwarte("Rechteck: eine Seite, +x", r.size() == 1U && r[0].seite == 0);
    erwarte("Rechteck: auf ein Viertel gerundet", r.size() == 1U && nah(r[0].s0, -0.25F) && nah(r[0].s1, 0.25F) && nah(r[0].t0, -0.25F) && nah(r[0].t1, 0.25F));
    // Daraus folgt das Feld durch ein Loch und die Fenster daneben.
    const float drin[3] = {1.0F, 0.0F, 0.2F};
    const float draussen[3] = {1.0F, 0.0F, 0.3F};
    const float links[3] = {1.0F, 0.3F, 0.0F};
    erwarte("Rechteck: neben der Flaeche, in der Rundung, zeigt Himmel", himmelDeckt(r, drin));
    erwarte("Rechteck: ausserhalb bleibt der Bildpuffer", !himmelDeckt(r, draussen) && !himmelDeckt(r, links));
    // Die Ecken liegen auf der Box und zeigen wieder in das Rechteck (MakeSkyVec gegen AddSkyPolygon).
    for (int seite = 0; seite < 5; ++seite) {
        float e[3];
        himmelEcke(seite, 0.5F, -0.25F, 1000.0F, e);
        HimmelRechteck q;
        q.seite = seite;
        q.s0 = 0.25F;
        q.s1 = 0.75F;
        q.t0 = -0.5F;
        q.t1 = 0.0F;
        erwarte("MakeSkyVec und AddSkyPolygon sind Umkehrungen", himmelDeckt({q}, e));
    }
    float s0 = 0.0F;
    float t0 = 0.0F;
    float s1 = 0.0F;
    float t1 = 0.0F;
    erwarte("DrawSkyBox: leere Seite", !himmelRunden(524288.0F, 524288.0F, -524288.0F, -524288.0F, s0, t0, s1, t1));
    erwarte("DrawSkyBox: auf -1..1 begrenzt", himmelRunden(-3.0F, 0.1F, 0.1F, 9.0F, s0, t0, s1, t1) && nah(s0, -1.0F) && nah(s1, 0.25F) && nah(t0, 0.0F) && nah(t1, 1.0F));
    erwarte("DrawSkyBox: nur jenseits des Randes zeichnet nichts", !himmelRunden(1.5F, 0.0F, 2.0F, 0.5F, s0, t0, s1, t1));
}

void gebiete() {
    BspGeometry g = grundWelt();
    BspMesh m;
    vorDemAuge(g, m, 1, -99.0F, 99.0F, -99.0F, 99.0F);   // eine Flaeche im Blatt hinter der Tuer
    HimmelBox b;
    b.anlegen(g, m, {kHimmelVorn});
    AreaPortale p;
    p.anlegen(2);
    HimmelSicht s = blickNachX();
    s.portale = &p;
    s.gebiet = 0;
    std::vector<HimmelRechteck> r;
    b.rechne(s, r);
    erwarte("R_MarkLeaves: hinter einem geschlossenen Portal zaehlt der Himmel nicht", r.empty() && b.flaechenImBild() == 0U);
    p.setze(0, 1, true);
    b.rechne(s, r);
    erwarte("R_MarkLeaves: Portal offen: er zaehlt", r.size() == 1U);
    p.setze(0, 1, false);
    s.gebiet = -1;
    b.rechne(s, r);
    erwarte("CM_SnapPVS: Auge in keinem Gebiet sieht alles", r.size() == 1U);
    s.gebiet = 0;
    s.portale = nullptr;
    b.rechne(s, r);
    erwarte("ohne Gebiete: alles", r.size() == 1U);
    // PVS
    g.vis = {0x01, 0x03};
    HimmelBox b2;
    b2.anlegen(g, m, {kHimmelVorn});
    HimmelSicht s2 = blickNachX();
    b2.rechne(s2, r);
    erwarte("R_MarkLeaves: ausserhalb des PVS zaehlt er nicht", r.empty());
    // R_ClusterPVS: ohne Tabelle sieht man alles
    g.vis.clear();
    HimmelBox b3;
    b3.anlegen(g, m, {kHimmelVorn});
    b3.rechne(s2, r);
    erwarte("R_ClusterPVS: ohne Tabelle alles", r.size() == 1U);
    // R_MarkLeaves: Auge ausserhalb (Cluster -1) markiert alles
    g.vis = {0x01, 0x03};
    g.leafs[0].cluster = -1;
    HimmelBox b4;
    b4.anlegen(g, m, {kHimmelVorn});
    b4.rechne(s2, r);
    erwarte("R_MarkLeaves: Cluster -1 sieht alles", r.size() == 1U);
}

void keulen() {
    BspGeometry g = grundWelt();
    BspMesh m;
    // Normale vom Auge weg: d = 1 gegen plane.dist 1000.
    const float e[4][3] = {{1000.0F, -99.0F, -99.0F}, {1000.0F, 99.0F, -99.0F}, {1000.0F, 99.0F, 99.0F}, {1000.0F, -99.0F, 99.0F}};
    const float n[3] = {1.0F, 0.0F, 0.0F};
    flaeche(g, m, 0, e, n);
    std::vector<HimmelRechteck> r;
    HimmelBox a;
    a.anlegen(g, m, {kHimmelVorn});
    a.rechne(blickNachX(), r);
    erwarte("R_CullSurface: cull front keult von hinten", r.empty());
    HimmelBox b;
    b.anlegen(g, m, {kHimmelHinten});
    b.rechne(blickNachX(), r);
    erwarte("R_CullSurface: cull back zeichnet von hinten", r.size() == 1U);
    HimmelBox c;
    c.anlegen(g, m, {kHimmelBeide});
    c.rechne(blickNachX(), r);
    erwarte("R_CullSurface: twosided zeichnet", r.size() == 1U);
    HimmelBox d;
    d.anlegen(g, m, {kKeinHimmel});
    d.rechne(blickNachX(), r);
    erwarte("Kein Himmel-Shader: nichts", d.leer() && r.empty());
}

void frustum() {
    BspGeometry g = grundWelt();
    BspMesh m;
    vorDemAuge(g, m, 0, -99.0F, 99.0F, -99.0F, 99.0F);
    g.leafs[0].mins[0] = 900.0F;
    g.leafs[0].maxs[0] = 1100.0F;
    for (int k = 1; k < 3; ++k) {
        g.leafs[0].mins[k] = -200.0F;
        g.leafs[0].maxs[k] = 200.0F;
    }
    g.nodes[0].mins[0] = -1200.0F;
    g.nodes[0].maxs[0] = 1200.0F;
    HimmelBox b;
    b.anlegen(g, m, {kHimmelVorn});
    std::vector<HimmelRechteck> r;
    b.rechne(blickNachX(), r);
    erwarte("R_RecursiveWorldNode: Blatt im Blick", r.size() == 1U);
    HimmelSicht zurueck = blickNachX(-1.0F);
    b.rechne(zurueck, r);
    erwarte("R_RecursiveWorldNode: Blatt hinter dem Blick zaehlt nicht", r.empty());
    HimmelSicht nah1 = blickNachX();
    nah1.distanceCull = 500.0F;
    b.rechne(nah1, r);
    erwarte("R_SetupFrustum: jenseits von distanceCull * 1,02 zaehlt nicht", r.empty());
    nah1.distanceCull = 900.0F / 1.02F + 1.0F;
    b.rechne(nah1, r);
    erwarte("R_SetupFrustum: knapp innerhalb zaehlt", r.size() == 1U);
}

// Endrunde rc608: die ferne Ebene fuer die Welt (fernStand, flaechenAusserSicht): Blatt 0 reicht von x 900 bis 1100, Blatt 1
// (x < 0) bleibt am Auge. Ein Dreiecksnetz im nahen Blatt 1, dessen Kasten ganz bei x -6000 liegt, faellt allein weg
// (R_CullSurface: R_CullTriSurf mit R_CullLocalBox); die ebene Flaeche daneben nur mit ihrem Blatt.
void fernebeneWelt() {
    BspGeometry g = grundWelt();
    BspMesh m;
    vorDemAuge(g, m, 0, -99.0F, 99.0F, -99.0F, 99.0F);
    vorDemAuge(g, m, 1, -99.0F, 99.0F, -99.0F, 99.0F, -500.0F);
    vorDemAuge(g, m, 1, -99.0F, 99.0F, -99.0F, 99.0F, -6000.0F);
    g.surfaces[2].type = BspSurface::Type::TriangleSoup;
    g.leafs[0].mins[0] = 900.0F;
    g.leafs[0].maxs[0] = 1100.0F;
    g.leafs[1].mins[0] = -6100.0F;
    g.leafs[1].maxs[0] = 0.0F;
    for (std::size_t b = 0; b < 2; ++b) {
        for (int k = 1; k < 3; ++k) {
            g.leafs[b].mins[k] = -200.0F;
            g.leafs[b].maxs[k] = 200.0F;
        }
    }
    BspGeometry::SubModel welt;
    for (int k = 0; k < 3; ++k) {
        welt.mins[k] = -200.0F;
        welt.maxs[k] = 200.0F;
    }
    welt.mins[0] = -6100.0F;
    welt.maxs[0] = 1100.0F;
    g.models.push_back(welt);
    const std::vector<FlaechenKasten> k = flaechenKaesten(g);
    erwarte("R_CullSurface: Kasten nur fuer Dreiecksnetz und Patch", !k[0].hat && !k[1].hat && k[2].hat && nah(k[2].mins[0], -6000.0F));
    const float auge[3] = {1.0F, 0.0F, 0.0F};
    const float vornX[3] = {1.0F, 0.0F, 0.0F};
    const float vornMinusX[3] = {-1.0F, 0.0F, 0.0F};
    AreaPortale p;
    FernStand f = fernStand(g, k, fernebeneFuer(auge, vornX, 12000.0F));
    erwarte("R_SetupFrustum: die Welt liegt ganz vor der fernen Ebene, nichts faellt weg", f.blaetter.empty() && f.flaechen.empty());
    f = fernStand(g, k, fernebeneFuer(auge, vornX, 500.0F));
    std::vector<char> v = flaechenAusserSicht(g, p, -1, -1, &f);
    erwarte("R_RecursiveWorldNode: Blatt jenseits von distanceCull * 1,02 faellt weg, das Blatt am Auge nicht", v.size() == 3U && v[0] == 1 && v[1] == 0 && v[2] == 0);
    f = fernStand(g, k, fernebeneFuer(auge, vornX, 900.0F / 1.02F + 1.0F));
    v = flaechenAusserSicht(g, p, -1, -1, &f);
    erwarte("R_SetupFrustum: knapp innerhalb bleibt das Blatt", v.size() == 3U && v[0] == 0);
    f = fernStand(g, k, fernebeneFuer(auge, vornMinusX, 3000.0F));
    v = flaechenAusserSicht(g, p, -1, -1, &f);
    erwarte("R_CullTriSurf: das Dreiecksnetz faellt allein weg, die ebene Flaeche im selben Blatt bleibt", v.size() == 3U && v[1] == 0 && v[2] == 1);
}

void unten() {
    BspGeometry g = grundWelt();
    BspMesh m;
    const float e[4][3] = {{100.0F, -50.0F, -500.0F}, {100.0F, 50.0F, -500.0F}, {200.0F, 50.0F, -500.0F}, {200.0F, -50.0F, -500.0F}};
    const float n[3] = {0.0F, 0.0F, 1.0F};
    flaeche(g, m, 0, e, n);
    HimmelBox b;
    b.anlegen(g, m, {kHimmelVorn});
    std::vector<HimmelRechteck> r;
    HimmelSicht s = blickNachX();
    s.vorn[0] = 0.0F;
    s.vorn[2] = -1.0F;
    s.hoch[0] = 1.0F;
    s.hoch[2] = 0.0F;
    b.rechne(s, r);
    erwarte("DrawSkyBox: \"still don't want to draw the bottom\"", r.empty() && b.flaechenImBild() == 1U);
}

void ueberlauf() {
    BspGeometry g = grundWelt();
    BspMesh m;
    // 250 mal 4 Ecken erreichen SHADER_MAX_VERTEXES: Die 250. Flaeche beginnt einen neuen Satz, jeder hat sein Rechteck.
    for (int i = 0; i < 249; ++i) { vorDemAuge(g, m, 0, -590.0F, -510.0F, -99.0F, 99.0F); }
    for (int i = 0; i < 10; ++i) { vorDemAuge(g, m, 0, 510.0F, 590.0F, -99.0F, 99.0F); }
    HimmelBox b;
    b.anlegen(g, m, {kHimmelVorn});
    std::vector<HimmelRechteck> r;
    b.rechne(blickNachX(), r);
    const float mitte[3] = {1.0F, 0.0F, 0.0F};
    const float einer[3] = {1.0F, 0.55F, 0.0F};
    const float anderer[3] = {1.0F, -0.55F, 0.0F};
    erwarte("RB_CHECKOVERFLOW: zwei Rechtecke, die Luecke bleibt", r.size() == 2U && !himmelDeckt(r, mitte) && himmelDeckt(r, einer) && himmelDeckt(r, anderer));
}

}  // namespace

int main() {
    rechteck();
    gebiete();
    keulen();
    frustum();
    fernebeneWelt();
    unten();
    ueberlauf();
    if (g_fehler != 0) {
        std::printf("himmelbox: %d Fehler\n", g_fehler);
        return 1;
    }
    std::printf("himmelbox: 0 Fehler\n");
    return 0;
}
