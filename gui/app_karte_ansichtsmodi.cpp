// app_karte_ansichtsmodi.cpp - Karten-Editor, Stufe 8b Punkt 5: Ansichtsmodi
// der Kamera (eigene Funktion, nicht in NetRadiant-Custom), einzeln schaltbar
// unter View > "Camera view modes":
//
//   Lightmap density  Schachbrett je Flaeche in der Groesse eines Luxels, wie
//                     q3map2 sie rechnet (surface.cpp ClassifySurfaces: Shader
//                     q3map_lightmapSampleSize vor _lightmapsamplesize/_samplesize/
//                     _ss der Entity vor -samplesize 16, mal lightmapscale/
//                     _lightmapscale/_ls, 1..16384; lightmaps_ydnar.cpp
//                     AllocateLightmapForSurface: + 1, bis die Flaeche in 128
//                     Luxel passt; ProjectSurfaceLightmap: die Achse nach der
//                     Normalen, z bevorzugt, das Raster an Vielfachen der
//                     Groesse). Feiner als 16 roetlich, groeber blaeulich; ohne
//                     Lightmap (nolightmap/pointlight, nodraw, sky) grau.
//                     Abweichung: q3map2 fasst Flaechen zu Roh-Lightmaps
//                     zusammen und vergroessert dann fuer alle - hier je Flaeche.
//   Collision         die Brushes nach ihrem Inhalt (SetBrushContents mit den
//                     surfaceparms des Spiels wie q3map2): playerclip magenta,
//                     monsterclip orange, botclip gelb, shotclip tuerkis, nodrop
//                     dunkelrot, Wasser blau, Schleim gruen, Lava orangerot,
//                     Nebel violett, Trigger gruen; sichtbare Waende ohne
//                     Kollision fuer den Spieler (nicht in MASK_PLAYERSOLID)
//                     leuchtend rot.
//   Visibility (PVS)  aus der gebauten .bsp neben der Karte: was die Engine vom
//                     Cluster der Kamera aus zeichnet (R_MarkLeaves: die
//                     Flaechen der Blaetter, deren Cluster im PVS liegt; ohne
//                     Cluster alles) - gruen ueber die Szene.
//   Triangle load     Waermekarte: je Flaeche der .bsp die Dreiecke, die die
//                     Engine aus ihrem Cluster zeichnet (die Summe ueber alle im
//                     PVS sichtbaren Flaechen), blau wenig, rot viel.
// Die Modi sind Anzeige - nichts wird gespeichert oder an der Karte geaendert.
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/bspgeo.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/patch.h"
#include "karte_gpu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

constexpr std::uint32_t S_CONT_SOLID = 0x1, S_CONT_LAVA = 0x2, S_CONT_WATER = 0x4, S_CONT_FOG = 0x8, S_CONT_PLAYERCLIP = 0x10,
                        S_CONT_MONSTERCLIP = 0x20, S_CONT_BOTCLIP = 0x40, S_CONT_SHOTCLIP = 0x80, S_CONT_TRIGGER = 0x400, S_CONT_NODROP = 0x800,
                        S_CONT_TERRAIN = 0x1000, S_CONT_SLIME = 0x20000;
constexpr std::uint32_t S_SURF_SKY = 0x2000, S_SURF_NODRAW = 0x200000;
constexpr std::uint32_t C_VERTEXLIT = 0x100, C_NODRAW = 0x10, C_SKY = 0x800;
constexpr std::uint32_t kMaskPlayerSolid = S_CONT_SOLID | S_CONT_PLAYERCLIP | 0x100 | S_CONT_TERRAIN;

struct Zwischen {
    std::size_t geoStand = static_cast<std::size_t>(-1);
    std::size_t ladeStand = static_cast<std::size_t>(-1);
    std::vector<kartegpu::Punkt3> dichte;
    std::vector<kartegpu::Punkt3> kollision;
    void* schachbrett = nullptr;
    unsigned bildStand = ~0U;
    // .bsp
    std::string bspPfad;
    std::filesystem::file_time_type bspZeit{};
    std::shared_ptr<BspGeometry> bsp;
    BspMesh netz;
    std::vector<std::uint32_t> dreieckeJeFlaeche;
    std::vector<std::uint64_t> lastJeCluster;
    std::uint64_t lastMax = 0;
    int sichtCluster = -3;
    std::vector<kartegpu::Punkt3> sicht;
    std::vector<kartegpu::Punkt3> last;
};
Zwischen& zw() {
    static Zwischen z;
    return z;
}

std::uint32_t farbe(float r, float g, float b, float a) {
    const auto k = [](float v) { return static_cast<std::uint32_t>(std::lround(std::clamp(v, 0.0F, 1.0F) * 255.0F)); };
    return k(r) | (k(g) << 8U) | (k(b) << 16U) | (k(a) << 24U);
}

kartegpu::Punkt3 punkt(float x, float y, float z, float s, float t, std::uint32_t f) {
    kartegpu::Punkt3 p{};
    p.pos[0] = x;
    p.pos[1] = y;
    p.pos[2] = z;
    p.normale[2] = 1;
    p.st[0] = s;
    p.st[1] = t;
    p.farbe = f;
    return p;
}

const karte::ShaderEintrag* shaderVon(const std::string& name) {
    const Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    return sp ? sp->inventar.shaderFinden(name) : nullptr;
}

bool istJk2() {
    const Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    return sp && sp->spiel == 3;   // kRegelwerkJediOutcast
}

karte::Q3ShaderFlags flagsVon(const std::string& shader) {
    const karte::ShaderEintrag* s = shaderVon(shader);
    return karte::q3map2ShaderFlags(s != nullptr ? s->surfaceparms : std::vector<std::string>{}, !istJk2());
}

// floatForKey / intForKey mit Ausweichschluesseln (q3map2 entitydata): der erste vorhandene
std::string wertAus(const karte::Entitaet& ent, std::initializer_list<const char*> schluessel) {
    for (const char* s : schluessel) {
        if (ent.hat(s)) { return ent.wert(s); }
    }
    return {};
}

// Die Luxelgroesse einer Flaeche (siehe Kopf)
int luxelGroesse(int shaderWert, int entitaetWert, float skala, const float mins[3], const float maxs[3]) {
    float ss = 0.0F;
    if (shaderWert > 0) {
        ss = static_cast<float>(shaderWert);
    } else if (entitaetWert > 0) {
        ss = static_cast<float>(entitaetWert);
    } else {
        ss = 16.0F;   // DEFAULT_LIGHTMAP_SAMPLE_SIZE (-samplesize)
    }
    if (skala > 0.0F) { ss = skala * ss; }
    ss = std::clamp(ss, 1.0F, 16384.0F);
    float g = static_cast<float>(static_cast<int>(ss));   // ds.sampleSize ist int
    for (int i = 0; i < 3; ++i) {   // AllocateLightmapForSurface: "hack (god this sucks)"
        const float lo = g * std::floor(mins[i] / g);
        const float hi = g * std::ceil(maxs[i] / g);
        const float groesse = (hi - lo) / g + 1.0F;
        if (groesse > 128.0F) {
            i = -1;
            g += 1.0F;
        }
    }
    return static_cast<int>(g);
}

void bspPruefen() {
    Zwischen& z = zw();
    const Editor& ed = e();
    std::string pfad = ed.name;
    const auto stelle = pfad.find_last_of('.');
    if (stelle != std::string::npos) { pfad = pfad.substr(0, stelle); }
    pfad += ".bsp";
    std::error_code ec;
    const std::filesystem::path p(std::u8string(reinterpret_cast<const char8_t*>(pfad.data()), pfad.size()));
    const auto zeit = std::filesystem::last_write_time(p, ec);
    if (ec) {
        if (z.bsp) {
            z.bsp.reset();
            z.netz = BspMesh{};
        }
        z.bspPfad = pfad;
        return;
    }
    if (z.bsp && z.bspPfad == pfad && zeit == z.bspZeit) { return; }
    z.bspPfad = pfad;
    z.bspZeit = zeit;
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    auto g = std::make_shared<BspGeometry>();
    if (!readBspGeometry(s.str(), *g)) {
        z.bsp.reset();
        return;
    }
    z.bsp = g;
    z.netz = buildModelMesh(*g, 0, 6);
    z.dreieckeJeFlaeche.assign(g->surfaces.size(), 0);
    for (const BspMesh::Run& r : z.netz.runs) {
        if (r.surface < z.dreieckeJeFlaeche.size()) { z.dreieckeJeFlaeche[r.surface] += r.numIndexes / 3; }
    }
    // Dreieckslast je Cluster: die Dreiecke aller von dort sichtbaren Flaechen
    z.lastJeCluster.assign(static_cast<std::size_t>(std::max(0, g->numClusters)), 0);
    z.lastMax = 0;
    for (int c = 0; c < g->numClusters; ++c) {
        std::uint64_t n = 0;
        for (std::size_t sfl = 0; sfl < z.dreieckeJeFlaeche.size(); ++sfl) {
            if (z.dreieckeJeFlaeche[sfl] != 0 && g->surfaceVisible(sfl, c)) { n += z.dreieckeJeFlaeche[sfl]; }
        }
        z.lastJeCluster[static_cast<std::size_t>(c)] = n;
        z.lastMax = std::max(z.lastMax, n);
    }
    z.sichtCluster = -3;
    z.last.clear();
}

// Die Dreiecke einer Flaeche der .bsp mit einer Farbe
void flaecheDazu(std::vector<kartegpu::Punkt3>& aus, const BspMesh::Run& r, std::uint32_t f) {
    const Zwischen& z = zw();
    for (std::uint32_t i = 0; i < r.numIndexes; ++i) {
        const BspVertex& v = z.netz.verts[z.netz.indexes[r.firstIndex + i]];
        aus.push_back(punkt(v.xyz[0], v.xyz[1], v.xyz[2], 0, 0, f));
    }
}

std::uint32_t waerme(float t) {   // 0 blau, 0.5 gruen/gelb, 1 rot
    t = std::clamp(t, 0.0F, 1.0F);
    const float r = std::clamp(2.0F * t, 0.0F, 1.0F);
    const float g = 1.0F - std::fabs(2.0F * t - 1.0F);
    const float b = std::clamp(1.0F - 2.0F * t, 0.0F, 1.0F);
    return farbe(r, g, b, 0.55F);
}

void kartenGeometrieBauen() {
    Zwischen& z = zw();
    Editor& ed = e();
    z.dichte.clear();
    z.kollision.clear();
    ed.ansichtsmodi.luxelJeShader.clear();
    ed.ansichtsmodi.dichteFlaechen = 0;
    ed.ansichtsmodi.kollisionFarbig = 0;
    ed.ansichtsmodi.ohneKollision = 0;
    for (const auto& ent : ed.karte.entitaeten()) {
        const std::string skalaText = wertAus(*ent, {"lightmapscale", "_lightmapscale", "_ls"});
        const float skala = skalaText.empty() ? 0.0F : std::max(0.0F, static_cast<float>(std::atof(skalaText.c_str())));
        const std::string ssText = wertAus(*ent, {"_lightmapsamplesize", "_samplesize", "_ss"});
        const int entSs = ssText.empty() ? 0 : std::max(0, std::atoi(ssText.c_str()));
        const bool trigger = ent->klassenname().rfind("trigger_", 0) == 0;
        for (const auto& prim : ent->kinder()) {
            if (prim->art() == karte::Primitiv::Art::Pinsel) {
                const auto& p = static_cast<const karte::Pinsel&>(*prim);
                p.bRep(ed.karte.kontext);
                std::vector<karte::Q3ShaderFlags> seiten;
                bool sichtbar = false;
                for (std::size_t i = 0; i < p.anzahl(); ++i) {
                    seiten.push_back(flagsVon(p[i].shader()));
                    if ((seiten.back().flaeche & S_SURF_NODRAW) == 0U && p[i].traegtBei()) { sichtbar = true; }
                }
                const karte::PinselInhalt pi = karte::pinselInhalt(seiten);
                std::uint32_t inhalt = pi.flags.inhalt;
                if (trigger) { inhalt = S_CONT_TRIGGER; }   // InitTrigger
                // Farbe der Kollision
                std::uint32_t kf = 0;
                if ((inhalt & S_CONT_PLAYERCLIP) != 0U && (inhalt & S_CONT_SOLID) == 0U) {
                    kf = farbe(1.0F, 0.0F, 1.0F, 0.5F);
                } else if ((inhalt & S_CONT_MONSTERCLIP) != 0U && (inhalt & S_CONT_SOLID) == 0U) {
                    kf = farbe(1.0F, 0.5F, 0.0F, 0.5F);
                } else if ((inhalt & S_CONT_BOTCLIP) != 0U) {
                    kf = farbe(1.0F, 1.0F, 0.0F, 0.5F);
                } else if ((inhalt & S_CONT_SHOTCLIP) != 0U) {
                    kf = farbe(0.0F, 1.0F, 1.0F, 0.5F);
                } else if ((inhalt & S_CONT_NODROP) != 0U) {
                    kf = farbe(0.55F, 0.0F, 0.0F, 0.55F);
                } else if ((inhalt & S_CONT_WATER) != 0U) {
                    kf = farbe(0.2F, 0.4F, 1.0F, 0.45F);
                } else if ((inhalt & S_CONT_SLIME) != 0U) {
                    kf = farbe(0.3F, 0.9F, 0.2F, 0.45F);
                } else if ((inhalt & S_CONT_LAVA) != 0U) {
                    kf = farbe(1.0F, 0.3F, 0.0F, 0.5F);
                } else if ((inhalt & S_CONT_FOG) != 0U) {
                    kf = farbe(0.6F, 0.3F, 0.9F, 0.4F);
                } else if ((inhalt & S_CONT_TRIGGER) != 0U) {
                    kf = farbe(0.2F, 0.85F, 0.3F, 0.4F);
                } else if ((inhalt & kMaskPlayerSolid) == 0U && sichtbar && !pi.ursprung) {
                    kf = farbe(1.0F, 0.1F, 0.1F, 0.7F);   // sichtbar, haelt aber nicht
                    ++ed.ansichtsmodi.ohneKollision;
                }
                if (kf != 0U) { ++ed.ansichtsmodi.kollisionFarbig; }
                for (std::size_t i = 0; i < p.anzahl(); ++i) {
                    const karte::Flaeche& f = p[i];
                    const karte::Wicklung& w = f.wicklung();
                    if (w.numpoints < 3) { continue; }
                    // Kollision
                    if (kf != 0U) {
                        for (std::size_t j = 1; j + 1 < w.numpoints; ++j) {
                            for (const std::size_t q : {static_cast<std::size_t>(0), j, j + 1}) {
                                const auto& v = w.points[q].vertex;
                                z.kollision.push_back(punkt(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]), 0, 0, kf));
                            }
                        }
                    }
                    // Lightmap-Dichte
                    if ((seiten[i].flaeche & S_SURF_NODRAW) != 0U && (seiten[i].compile & C_NODRAW) != 0U) { continue; }
                    float mins[3] = {1e30F, 1e30F, 1e30F};
                    float maxs[3] = {-1e30F, -1e30F, -1e30F};
                    for (std::size_t j = 0; j < w.numpoints; ++j) {
                        for (int a = 0; a < 3; ++a) {
                            mins[a] = std::min(mins[a], static_cast<float>(w.points[j].vertex[static_cast<std::size_t>(a)]));
                            maxs[a] = std::max(maxs[a], static_cast<float>(w.points[j].vertex[static_cast<std::size_t>(a)]));
                        }
                    }
                    const bool ohneLightmap = (seiten[i].compile & (C_VERTEXLIT | C_SKY)) != 0U || (seiten[i].flaeche & S_SURF_SKY) != 0U;
                    const karte::ShaderEintrag* se = shaderVon(f.shader());
                    const int ss = luxelGroesse(se != nullptr ? se->lightmapAbtastung : 0, entSs, skala, mins, maxs);
                    {
                        int& alt = ed.ansichtsmodi.luxelJeShader[f.shader()];
                        alt = std::max(alt, ohneLightmap ? 0 : ss);
                    }
                    const karte::Ebene3d& eb = f.ebene();
                    const double ax = std::fabs(eb.a);
                    const double ay = std::fabs(eb.b);
                    const double az = std::fabs(eb.c);
                    const int achse = (az >= ax && az >= ay) ? 2 : (ax >= ay && ax >= az) ? 0 : 1;
                    std::uint32_t df = farbe(0.6F, 0.6F, 0.6F, 0.5F);
                    if (!ohneLightmap) {
                        const float t = std::clamp(std::log2(static_cast<float>(ss) / 16.0F) / 3.0F, -1.0F, 1.0F);   // 2 .. 128
                        df = t < 0 ? farbe(1.0F, 1.0F + t * 0.7F, 1.0F + t * 0.9F, 0.55F) : farbe(1.0F - t * 0.8F, 1.0F - t * 0.5F, 1.0F, 0.55F);
                        ++ed.ansichtsmodi.dichteFlaechen;
                    }
                    const auto uv = [&](const karte::Vektor3d& v, float& u, float& vv) {
                        const float x = static_cast<float>(v[0]);
                        const float y = static_cast<float>(v[1]);
                        const float zz = static_cast<float>(v[2]);
                        const float a = achse == 2 ? x : achse == 0 ? y : x;
                        const float b = achse == 2 ? y : zz;
                        u = a / (2.0F * static_cast<float>(ss));
                        vv = b / (2.0F * static_cast<float>(ss));
                    };
                    for (std::size_t j = 1; j + 1 < w.numpoints; ++j) {
                        for (const std::size_t q : {static_cast<std::size_t>(0), j, j + 1}) {
                            const auto& v = w.points[q].vertex;
                            float u = 0;
                            float vv = 0;
                            uv(v, u, vv);
                            z.dichte.push_back(punkt(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]), ohneLightmap ? 0.25F : u,
                                                     ohneLightmap ? 0.25F : vv, df));
                        }
                    }
                }
            } else if (prim->art() == karte::Primitiv::Art::Patch) {
                const auto& pa = static_cast<const karte::Patch&>(*prim);
                const karte::Q3ShaderFlags fl = flagsVon(pa.shader());
                std::uint32_t kf = 0;
                const std::uint32_t inhalt = trigger ? S_CONT_TRIGGER : fl.inhalt;
                if ((inhalt & S_CONT_PLAYERCLIP) != 0U && (inhalt & S_CONT_SOLID) == 0U) {
                    kf = farbe(1.0F, 0.0F, 1.0F, 0.5F);
                } else if ((inhalt & kMaskPlayerSolid) == 0U && (fl.flaeche & S_SURF_NODRAW) == 0U) {
                    kf = farbe(1.0F, 0.1F, 0.1F, 0.7F);
                    ++ed.ansichtsmodi.ohneKollision;
                }
                const karte::PatchTesselierung& t = pa.tesselierung();
                if (kf != 0U) {
                    for (std::size_t s = 0; s < t.numStrips; ++s) {
                        for (std::size_t i = 0; i + 2 < t.lenStrips; ++i) {
                            const std::size_t b = s * t.lenStrips;
                            for (const std::size_t q : {b + i, b + i + 1, b + i + 2}) {
                                const karte::Vektor3f& v = t.vertices[t.indices[q]].vertex;
                                z.kollision.push_back(punkt(v[0], v[1], v[2], 0, 0, kf));
                            }
                        }
                    }
                }
            }
        }
    }
    z.geoStand = geometrieStand();
    z.ladeStand = ed.ladeZaehler;
}

}  // namespace

void ansichtsmodiZeichnen(const float* matrix, const float auge[3]) {
    Editor& ed = e();
    AnsichtsModiZustand& m = ed.ansichtsmodi;
    if (!m.dichte && !m.kollision && !m.sichtbarkeit && !m.last) { return; }
    Zwischen& z = zw();
    if ((m.dichte || m.kollision) && (z.geoStand != geometrieStand() || z.ladeStand != ed.ladeZaehler)) { kartenGeometrieBauen(); }
    if (z.schachbrett == nullptr || z.bildStand != kartegpu::geraetStand()) {
        // 64 x 64, vier Felder zu 32: hell / dunkel
        std::vector<unsigned char> rgba(64 * 64 * 4);
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 64; ++x) {
                const bool hell = ((x / 32) + (y / 32)) % 2 == 0;
                const std::size_t i = static_cast<std::size_t>((y * 64 + x) * 4);
                rgba[i] = rgba[i + 1] = rgba[i + 2] = hell ? 255 : 110;
                rgba[i + 3] = 255;
            }
        }
        z.schachbrett = kartegpu::bildAnlegen(rgba.data(), 64, 64, 0);
        z.bildStand = kartegpu::geraetStand();
    }
    kartegpu::Zustand3 st;
    st.matrix = matrix;
    st.tiefe = kartegpu::TiefenTest::KleinerGleich;
    st.tiefeSchreiben = false;
    st.wegschneiden = true;
    st.mischen = kartegpu::Mischen::Alpha;
    st.versatz = true;
    for (int a = 0; a < 3; ++a) { st.auge[a] = auge[a]; }
    if (m.dichte && !z.dichte.empty()) {
        kartegpu::Zustand3 d = st;
        d.bild = z.schachbrett;
        kartegpu::dreiecke(z.dichte, d);
    }
    if (m.kollision && !z.kollision.empty()) {
        kartegpu::Zustand3 k = st;
        k.wegschneiden = false;
        kartegpu::dreiecke(z.kollision, k);
    }
    if (m.sichtbarkeit || m.last) {
        bspPruefen();
        m.bspDa = static_cast<bool>(z.bsp);
        if (!z.bsp) { return; }
        const BspGeometry& g = *z.bsp;
        const int blatt = g.leafAt(auge);
        const int cluster = blatt >= 0 && blatt < static_cast<int>(g.leafs.size()) ? g.leafs[static_cast<std::size_t>(blatt)].cluster : -1;
        m.kameraCluster = cluster;
        m.clusterZahl = g.numClusters;
        if (cluster != z.sichtCluster) {
            z.sichtCluster = cluster;
            z.sicht.clear();
            m.sichtbareCluster = 0;
            m.sichtbareDreiecke = 0;
            for (int c = 0; c < g.numClusters; ++c) { m.sichtbareCluster += (cluster < 0 || g.clusterVisible(cluster, c)) ? 1 : 0; }
            for (const BspMesh::Run& r : z.netz.runs) {
                if (cluster >= 0 && !g.surfaceVisible(r.surface, cluster)) { continue; }
                flaecheDazu(z.sicht, r, farbe(0.1F, 1.0F, 0.3F, 0.35F));
                m.sichtbareDreiecke += r.numIndexes / 3;
            }
        }
        m.kameraLast = cluster >= 0 && cluster < static_cast<int>(z.lastJeCluster.size()) ? z.lastJeCluster[static_cast<std::size_t>(cluster)] : 0;
        m.lastMax = z.lastMax;
        if (m.sichtbarkeit && !z.sicht.empty()) {
            kartegpu::Zustand3 s = st;
            s.wegschneiden = false;
            kartegpu::dreiecke(z.sicht, s);
        }
        if (m.last) {
            if (z.last.empty()) {
                for (const BspMesh::Run& r : z.netz.runs) {
                    std::uint64_t l = 0;
                    if (r.surface < g.surfaceFirst.size()) {
                        const std::uint32_t erst = g.surfaceFirst[r.surface];
                        for (std::uint32_t i = 0; i < g.surfaceCount[r.surface]; ++i) {
                            const int c = g.surfaceClusters[erst + i];
                            if (c >= 0 && c < static_cast<int>(z.lastJeCluster.size())) { l = std::max(l, z.lastJeCluster[static_cast<std::size_t>(c)]); }
                        }
                    }
                    flaecheDazu(z.last, r, waerme(z.lastMax > 0 ? static_cast<float>(l) / static_cast<float>(z.lastMax) : 0.0F));
                }
            }
            kartegpu::Zustand3 s = st;
            s.wegschneiden = false;
            kartegpu::dreiecke(z.last, s);
        }
    }
}

void ansichtsmodiAnzeige(void* drawList, float x, float y) {
    Editor& ed = e();
    const AnsichtsModiZustand& m = ed.ansichtsmodi;
    auto* dl = static_cast<ImDrawList*>(drawList);
    float zy = y;
    const auto zeile = [&](const char* t, ImU32 f) {
        dl->AddRectFilled(ImVec2{x - 2, zy - 1}, ImVec2{x + ImGui::CalcTextSize(t).x + 2, zy + ImGui::GetTextLineHeight()}, IM_COL32(0, 0, 0, 150));
        dl->AddText(ImVec2{x, zy}, f, t);
        zy += ImGui::GetTextLineHeight() + 2;
    };
    char t[256];
    if (m.dichte) {
        std::snprintf(t, sizeof(t), "%s", trK(Str::KeAmDichteHud));
        zeile(t, IM_COL32(255, 255, 255, 255));
    }
    if (m.kollision) {
        std::snprintf(t, sizeof(t), trK(Str::KeAmKollisionHud), m.ohneKollision);
        zeile(t, IM_COL32(255, 160, 255, 255));
    }
    if (m.sichtbarkeit || m.last) {
        if (!m.bspDa) {
            zeile(trK(Str::KeAmKeineBsp), IM_COL32(255, 120, 120, 255));
        } else {
            if (m.sichtbarkeit) {
                std::snprintf(t, sizeof(t), trK(Str::KeAmSichtHud), m.kameraCluster, m.sichtbareCluster, m.clusterZahl, static_cast<unsigned long long>(m.sichtbareDreiecke));
                zeile(t, IM_COL32(120, 255, 150, 255));
            }
            if (m.last) {
                std::snprintf(t, sizeof(t), trK(Str::KeAmLastHud), static_cast<unsigned long long>(m.kameraLast), static_cast<unsigned long long>(m.lastMax));
                zeile(t, IM_COL32(255, 210, 120, 255));
            }
        }
    }
}

}  // namespace bhed::gui::karteneditor
