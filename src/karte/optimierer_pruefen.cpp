// optimierer_pruefen.cpp - Karten-Editor, Stufe 5 Punkt 3: der Optimierer
// sucht Bauprobleme (siehe optimierer.h).
//
// Portiert aus OpenJK und q3map2 (die Stellen, an denen Engine und Compiler
// das jeweilige Problem melden oder verursachen, stehen bei jeder Pruefung):
//   q3map2 (NetRadiant-Custom 20260114): leakfile.cpp (.lin), map.cpp
//   (SetBrushContents "Mixed detail and structural", "areaportals only allowed
//   in world"), facebsp.cpp (Schnitte am Blockgitter), tjunction.cpp
//   (FixTJunctions), model.cpp (Clip-Modelle), bspfile_rbsp.cpp (volles
//   Lichtgitter), lightmaps_ydnar.cpp (_lightmapscale);
//   OpenJK: code/server/sv_game.cpp (SV_SetBrushModel: NULL),
//   code/qcommon/cm_test.cpp (CM_PointLeafnum), cm_patch.cpp, cm_load.cpp,
//   code/rd-vanilla/tr_bsp.cpp (R_LoadLightGridArray).
// Dazu die Faustregeln der Mapping-Tutorials (JKHub "Mapping Errors List"
// r142, "Reducing passage portal flow" r147, RichDiesal) - als solche
// gekennzeichnet (schaetzung).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/karte/optimierer.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patchkollision.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
bool beginntMit(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }
int ganz(const std::string& s) { return std::atoi(s.c_str()); }

constexpr std::size_t kMaxObjekte = 2000000;   // so viele Objekte je Befund (die Behebung braucht alle)

bool weltArtig(const AbbildEntitaet& e) { return e.klasse == "worldspawn" || e.klasse == "func_group"; }

const ShaderArt& artVon(const PruefOptionen& o, const std::string& shader, std::map<std::string, ShaderArt>& vorrat) {
    const std::string k = klein(shader);
    const auto it = o.shader.find(k);
    if (it != o.shader.end()) { return it->second; }
    auto v = vorrat.find(k);
    if (v == vorrat.end()) { v = vorrat.emplace(k, shaderArtNachName(k)).first; }
    return v->second;
}

std::string stelle(const AbbildEntitaet& e, int ent, int prim) {
    return e.klasse + " #" + std::to_string(ent) + (prim >= 0 ? ", primitive " + std::to_string(prim) : std::string());
}

// Liegt p im (konvexen) Pinsel? Die Normalen der Flaechen zeigen nach aussen.
bool imPinsel(const AbbildPrimitiv& p, const Vektor3d& v, double eps) {
    for (const AbbildFlaeche& f : p.flaechen) {
        if (abstandZuPunkt(f.ebene, v) > -eps) { return false; }
    }
    return !p.flaechen.empty();
}

// --- 2D-Polygon auf der Ebene ---
struct P2 {
    double x;
    double y;
};
std::vector<P2> projiziert(const std::vector<Vektor3d>& pts, const Vektor3d& n) {
    // die groesste Komponente der Normalen weglassen
    const double ax = std::fabs(n[0]);
    const double ay = std::fabs(n[1]);
    const double az = std::fabs(n[2]);
    int a = 0;
    int b = 1;
    if (ax >= ay && ax >= az) {
        a = 1;
        b = 2;
    } else if (ay >= ax && ay >= az) {
        a = 0;
        b = 2;
    }
    std::vector<P2> r;
    r.reserve(pts.size());
    for (const Vektor3d& p : pts) { r.push_back({p[static_cast<std::size_t>(a)], p[static_cast<std::size_t>(b)]}); }
    return r;
}
double flaeche2(const std::vector<P2>& p) {
    double s = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const P2& a = p[i];
        const P2& b = p[(i + 1) % p.size()];
        s += a.x * b.y - b.x * a.y;
    }
    return s * 0.5;
}
// Liegt q im konvexen Polygon g (Umlaufsinn orient), mit Toleranz eps (Einheiten)?
bool imPolygon(const std::vector<P2>& g, double orient, const P2& q, double eps) {
    for (std::size_t i = 0; i < g.size(); ++i) {
        const P2& a = g[i];
        const P2& b = g[(i + 1) % g.size()];
        const double ex = b.x - a.x;
        const double ey = b.y - a.y;
        const double len = std::sqrt(ex * ex + ey * ey);
        if (len < 1e-9) { continue; }
        const double kreuz = (ex * (q.y - a.y) - ey * (q.x - a.x)) / len;   // vorzeichenbehafteter Abstand zur Kante
        if (kreuz * orient < -eps) { return false; }
    }
    return true;
}

// Schluessel einer Ebene (Normale auf 1/1000, Abstand auf 1/8)
struct EbenenSchluessel {
    long long nx, ny, nz, d;
    bool operator==(const EbenenSchluessel& o) const { return nx == o.nx && ny == o.ny && nz == o.nz && d == o.d; }
};
struct EbenenHash {
    std::size_t operator()(const EbenenSchluessel& k) const {
        return static_cast<std::size_t>(k.nx * 73856093LL ^ k.ny * 19349663LL ^ k.nz * 83492791LL ^ k.d * 2654435761LL);
    }
};
EbenenSchluessel schluesselVon(const Ebene3d& e, bool umgedreht, long long dVersatz = 0) {
    const double s = umgedreht ? -1.0 : 1.0;
    return {std::llround(e.a * s * 1000), std::llround(e.b * s * 1000), std::llround(e.c * s * 1000), std::llround(e.d * s * 8) + dVersatz};
}

struct FlaechenRef {
    int ent;
    int prim;
    int fl;
    double mn[3];
    double mx[3];
};
void grenzenVon(const std::vector<Vektor3d>& pts, double* mn, double* mx) {
    for (int i = 0; i < 3; ++i) {
        mn[i] = 1e30;
        mx[i] = -1e30;
    }
    for (const Vektor3d& p : pts) {
        for (std::size_t i = 0; i < 3; ++i) {
            mn[i] = std::min(mn[i], p[i]);
            mx[i] = std::max(mx[i], p[i]);
        }
    }
}

}  // namespace

ShaderArt shaderArtNachName(const std::string& shader) {
    const std::string s = klein(shader);
    ShaderArt a;
    const bool common = beginntMit(s, "textures/common/") || beginntMit(s, "textures/system/");
    if (common) {
        const std::string n = s.substr(s.find('/', 9) + 1);
        if (n == "caulk" || n == "caulkshadow" || n == "nodraw" || n == "nodrawsolid" || n == "terrain" || n == "nodraw_solid") {
            a.sichtbar = false;   // fest und deckend, wird nicht gezeichnet
            return a;
        }
        if (n.find("sky") != std::string::npos) { return a; }   // Himmel: gezeichnet, deckend
        a.werkzeug = true;
        a.sichtbar = false;
        a.undurchsichtig = false;   // clip, trigger, hint, skip, areaportal, origin, nodrawnonsolid, ...
        return a;
    }
    for (const char* t : {"glass", "water", "fog", "trans", "window", "alpha", "grate", "fence", "liquid", "lava", "slime", "force_field", "forcefield"}) {
        if (s.find(t) != std::string::npos) {
            a.undurchsichtig = false;
            break;
        }
    }
    return a;
}

const char* schwereName(Schwere s) {
    switch (s) {
    case Schwere::Hinweis: return "note";
    case Schwere::Warnung: return "warning";
    case Schwere::Fehler: return "error";
    }
    return "";
}

std::vector<Befund> pruefeKarte(const KartenAbbild& a, const PruefOptionen& o) {
    std::vector<Befund> out;
    std::map<std::string, ShaderArt> vorrat;
    const auto& ents = a.ents;
    auto neu = [](const char* id, Schwere s, const char* quelle) {
        Befund b;
        b.pruefung = id;
        b.schwere = s;
        b.quelle = quelle;
        return b;
    };
    auto add = [&](Befund& b, const ObjektRef& r) {
        ++b.anzahl;
        if (b.objekte.size() < kMaxObjekte) { b.objekte.push_back(r); }
    };
    auto uhr = std::chrono::steady_clock::now();
    auto messen = [&](const char* was) {
        const auto jetzt = std::chrono::steady_clock::now();
        if (o.zeiten != nullptr) { o.zeiten->emplace_back(was, std::chrono::duration<double, std::milli>(jetzt - uhr).count()); }
        uhr = jetzt;
    };

    // --- 1. Leck (q3map2 leakfile.cpp: die .lin) und Detail am Leck -----------------
    if (!o.linDaten.empty()) {
        std::vector<Vektor3d> punkte;
        std::istringstream in(o.linDaten);
        double x = 0;
        double y = 0;
        double z = 0;
        while (in >> x >> y >> z) { punkte.emplace_back(x, y, z); }
        Befund b = neu("leck", Schwere::Fehler, "q3map2 leakfile.cpp LeakFile (.lin), bsp.cpp \"leaked\"; VIS needs a sealed hull");
        b.anzahl = static_cast<long long>(punkte.size());
        b.detail = std::to_string(punkte.size()) + " points in the leak line";
        out.push_back(b);
        // Detail-Pinsel, durch die die Linie laeuft: detail versiegelt nichts
        Befund d = neu("detailamleck", Schwere::Warnung, "q3map2 map.cpp SetBrushContents: C_DETAIL brushes are not used for the BSP (tutorial r142: detail at the void leaks)");
        d.behebung = Behebung::Struktur;
        std::set<std::pair<int, int>> gefunden;
        for (std::size_t i = 0; i + 1 < punkte.size(); ++i) {
            const Vektor3d& p0 = punkte[i];
            const Vektor3d& p1 = punkte[i + 1];
            const double len = std::sqrt(laengeQuadrat(p1 - p0));
            const int schritte = std::max(1, static_cast<int>(len / 8.0));
            for (int s = 0; s <= schritte; ++s) {
                const Vektor3d p = p0 + (p1 - p0) * (static_cast<double>(s) / schritte);
                for (std::size_t e = 0; e < ents.size(); ++e) {
                    for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                        const AbbildPrimitiv& pr = ents[e].prims[k];
                        if (pr.patch || !pr.detail || pr.entartet) { continue; }
                        if (p[0] < pr.mins[0] - 1 || p[0] > pr.maxs[0] + 1 || p[1] < pr.mins[1] - 1 || p[1] > pr.maxs[1] + 1 || p[2] < pr.mins[2] - 1 ||
                            p[2] > pr.maxs[2] + 1) {
                            continue;
                        }
                        if (imPinsel(pr, p, -0.5) && gefunden.insert({static_cast<int>(e), static_cast<int>(k)}).second) {
                            add(d, ObjektRef{static_cast<int>(e), static_cast<int>(k), -1, stelle(ents[e], static_cast<int>(e), static_cast<int>(k))});
                        }
                    }
                }
            }
        }
        if (d.anzahl > 0) {
            d.detail = std::to_string(d.anzahl) + " detail brushes on the leak line";
            out.push_back(d);
        }
    }

    messen("leck");
    // --- 2. Verdeckte Flaechen, nicht gecaulkt ------------------------------------------
    // Eine sichtbare Flaeche, die ganz von einer deckenden Flaeche eines
    // anderen Pinsels (gleiche Ebene, Gegenrichtung) bedeckt ist, wird nie
    // gesehen - q3map2 erzeugt fuer sie trotzdem eine Zeichenflaeche und
    // Lightmap-Platz (nur Strukturpinsel fallen in Blaetter im Festen).
    {
        Befund b = neu("verdeckt", Schwere::Hinweis, "q3map2 surface.cpp (draw surface per visible brush side), tutorial: build in caulk, AutoCaulk");
        b.behebung = Behebung::Caulk;
        b.sicher = true;
        std::unordered_map<EbenenSchluessel, std::vector<FlaechenRef>, EbenenHash> deckend;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            if (!weltArtig(ents[e])) { continue; }
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch || pr.entartet) { continue; }
                for (std::size_t f = 0; f < pr.flaechen.size(); ++f) {
                    const AbbildFlaeche& fl = pr.flaechen[f];
                    if (fl.punkte.size() < 3 || !artVon(o, fl.shader, vorrat).undurchsichtig) { continue; }
                    FlaechenRef r{static_cast<int>(e), static_cast<int>(k), static_cast<int>(f), {0, 0, 0}, {0, 0, 0}};
                    grenzenVon(fl.punkte, r.mn, r.mx);
                    deckend[schluesselVon(fl.ebene, false)].push_back(r);
                }
            }
        }
        long long flaechen = 0;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            if (!weltArtig(ents[e])) { continue; }
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch || pr.entartet) { continue; }
                for (std::size_t f = 0; f < pr.flaechen.size(); ++f) {
                    const AbbildFlaeche& fl = pr.flaechen[f];
                    if (fl.punkte.size() < 3 || !artVon(o, fl.shader, vorrat).sichtbar) { continue; }
                    const std::vector<P2> mein = projiziert(fl.punkte, fl.ebene.normal());
                    double mn[3];
                    double mx[3];
                    grenzenVon(fl.punkte, mn, mx);
                    bool verdeckt = false;
                    for (long long dv = -1; dv <= 1 && !verdeckt; ++dv) {
                        const auto it = deckend.find(schluesselVon(fl.ebene, true, dv));
                        if (it == deckend.end()) { continue; }
                        for (const FlaechenRef& g : it->second) {
                            if (g.ent == static_cast<int>(e) && g.prim == static_cast<int>(k)) { continue; }
                            if (mn[0] < g.mn[0] - 0.1 || mn[1] < g.mn[1] - 0.1 || mn[2] < g.mn[2] - 0.1 || mx[0] > g.mx[0] + 0.1 || mx[1] > g.mx[1] + 0.1 ||
                                mx[2] > g.mx[2] + 0.1) {
                                continue;   // F liegt nicht im Kasten von G
                            }
                            const AbbildFlaeche& gf = ents[static_cast<std::size_t>(g.ent)].prims[static_cast<std::size_t>(g.prim)].flaechen[static_cast<std::size_t>(g.fl)];
                            // wirklich dieselbe Ebene in Gegenrichtung?
                            if (std::fabs(punkt(gf.ebene.normal(), fl.ebene.normal()) + 1.0) > 1e-4 || std::fabs(gf.ebene.d + fl.ebene.d) > 0.05) { continue; }
                            const std::vector<P2> poly = projiziert(gf.punkte, fl.ebene.normal());
                            const double orient = flaeche2(poly) >= 0 ? 1.0 : -1.0;
                            bool alle = true;
                            for (const P2& q : mein) {
                                if (!imPolygon(poly, orient, q, 0.05)) {
                                    alle = false;
                                    break;
                                }
                            }
                            if (alle) {
                                verdeckt = true;
                                break;
                            }
                        }
                    }
                    if (verdeckt) {
                        ++flaechen;
                        ObjektRef r{static_cast<int>(e), static_cast<int>(k), -1,
                                    stelle(ents[e], static_cast<int>(e), static_cast<int>(k)) + ", face " + std::to_string(f) + " " + fl.shader, static_cast<int>(f)};
                        if (b.objekte.size() < kMaxObjekte) { b.objekte.push_back(r); }
                    }
                }
            }
        }
        b.anzahl = flaechen;
        if (flaechen > 0) {
            b.detail = std::to_string(flaechen) + " visible faces completely covered by an opaque face of another brush";
            out.push_back(b);
        }
    }

    messen("verdeckt");
    // --- 3. Entartete, Mikro- und Off-Grid-Pinsel, Mischinhalt ------------------------------
    {
        Befund ent = neu("entartet", Schwere::Warnung, "q3map2 brush.cpp CreateBrushWindings (degenerate brush removed); NRC brush.cpp buildWindings");
        ent.behebung = Behebung::PinselEntfernen;
        ent.sicher = true;   // existieren im Spiel ohnehin nicht
        Befund mikro = neu("mikro", Schwere::Warnung, "tutorial r142 (microbrushes: BSP precision, MAX_POINTS_ON_WINDING); q3map2 map.cpp");
        mikro.behebung = Behebung::PinselEntfernen;
        Befund raster = neu("offgrid", Schwere::Hinweis, "q3map2 map.cpp FindFloatPlane (plane snapping), tutorial: keep vertices on the grid (cracks, sparklies)");
        raster.behebung = Behebung::Einrasten;
        raster.sicher = true;
        double rasterMax = 0;
        Befund misch = neu("mischinhalt", Schwere::Hinweis, "q3map2 map.cpp:509 \"Mixed detail and structural (defaulting to structural)\"");
        misch.behebung = Behebung::Struktur;
        misch.sicher = true;
        Befund portalEnt = neu("areaportalentity", Schwere::Fehler, "q3map2 map.cpp:810 \"areaportals only allowed in world\"");
        for (std::size_t e = 0; e < ents.size(); ++e) {
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch) { continue; }
                const ObjektRef r{static_cast<int>(e), static_cast<int>(k), -1, stelle(ents[e], static_cast<int>(e), static_cast<int>(k))};
                if (pr.entartet || pr.flaechen.size() < 4) {
                    add(ent, r);
                    continue;
                }
                const double ausdehnung = std::min({pr.maxs[0] - pr.mins[0], pr.maxs[1] - pr.mins[1], pr.maxs[2] - pr.mins[2]});
                double abw = 0;
                if (ausdehnung < 1.0) {
                    // duenner als das Raster: Einrasten kann ihn nur zerdruecken -
                    // er zaehlt als Mikropinsel (Behebung: loeschen), nicht als "offgrid"
                    add(mikro, r);
                } else {
                    for (const AbbildFlaeche& f : pr.flaechen) {
                        for (const Vektor3d& v : f.punkte) {
                            for (std::size_t i = 0; i < 3; ++i) { abw = std::max(abw, std::fabs(v[i] - std::round(v[i]))); }
                        }
                    }
                }
                if (abw > 0.001) {
                    add(raster, r);
                    rasterMax = std::max(rasterMax, abw);
                }
                bool hint = false;
                bool portal = false;
                for (const AbbildFlaeche& f : pr.flaechen) {
                    const std::string s = klein(f.shader);
                    if (s == "textures/common/hint" || s == "textures/common/hintskip" || s == "textures/system/hint") { hint = true; }
                    if (s == "textures/common/areaportal" || s == "textures/system/areaportal") { portal = true; }
                }
                if ((hint || portal) && pr.detail) { add(misch, r); }
                if (portal && !weltArtig(ents[e])) { add(portalEnt, r); }
            }
        }
        if (ent.anzahl > 0) {
            ent.detail = std::to_string(ent.anzahl) + " brushes without a valid shape (fewer than 4 faces or no windings)";
            out.push_back(ent);
        }
        if (mikro.anzahl > 0) {
            mikro.detail = std::to_string(mikro.anzahl) + " brushes thinner than 1 unit";
            out.push_back(mikro);
        }
        if (raster.anzahl > 0) {
            char z[64];
            std::snprintf(z, sizeof(z), "%.3f", rasterMax);
            raster.detail = std::to_string(raster.anzahl) + " brushes with vertices off the integer grid (largest deviation " + z + ")";
            raster.sicher = rasterMax <= 0.05;   // nur Rundungsrauschen gilt als sicher
            out.push_back(raster);
        }
        if (misch.anzahl > 0) {
            misch.detail = std::to_string(misch.anzahl) + " detail brushes with hint or areaportal faces";
            out.push_back(misch);
        }
        if (portalEnt.anzahl > 0) {
            portalEnt.detail = std::to_string(portalEnt.anzahl) + " areaportal brushes in entities";
            out.push_back(portalEnt);
        }
    }

    messen("pinsel");
    // --- 4. Deko-Pinsel als structural (Vorschlag) ------------------------------------------
    {
        Befund b = neu("deko", Schwere::Hinweis,
                       "q3map2 map.cpp (structural brushes split the BSP); tutorial r142 \"Leaf with too many portals\", RichDiesal: details as detail; NRC#109 (vis errors and black lightmap spots from many small or angled structural brushes)");
        b.behebung = Behebung::Detail;
        b.schaetzung = true;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            if (!weltArtig(ents[e])) { continue; }
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch || pr.detail || pr.entartet || pr.flaechen.size() < 4) { continue; }
                bool werkzeug = false;
                bool sichtbar = false;
                bool schraeg = false;
                for (const AbbildFlaeche& f : pr.flaechen) {
                    const ShaderArt& sa = artVon(o, f.shader, vorrat);
                    if (sa.werkzeug) { werkzeug = true; }
                    if (sa.sichtbar) { sichtbar = true; }
                    const Vektor3d n = f.ebene.normal();
                    if (std::fabs(n[0]) < 0.999 && std::fabs(n[1]) < 0.999 && std::fabs(n[2]) < 0.999) { schraeg = true; }
                }
                if (werkzeug || !sichtbar) { continue; }
                const double dx = pr.maxs[0] - pr.mins[0];
                const double dy = pr.maxs[1] - pr.mins[1];
                const double dz = pr.maxs[2] - pr.mins[2];
                const double gross = std::max({dx, dy, dz});
                const double klein_ = std::min({dx, dy, dz});
                const bool winzig = gross <= 64;
                const bool duenn = klein_ <= 8 && gross <= 256;
                const bool verwinkelt = (pr.flaechen.size() > 6 || schraeg) && gross <= 256;
                if (winzig || duenn || verwinkelt) {
                    add(b, ObjektRef{static_cast<int>(e), static_cast<int>(k), -1, stelle(ents[e], static_cast<int>(e), static_cast<int>(k))});
                }
            }
        }
        if (b.anzahl > 0) {
            b.detail = std::to_string(b.anzahl) + " small, thin or angled structural brushes (suggestion: detail)";
            out.push_back(b);
        }
    }

    messen("deko");
    // --- 5. Ueberlappende Pinsel und Schnitte am Blockgitter ---------------------------------
    {
        struct Kasten3 {
            int ent;
            int prim;
            Vektor3f mn;
            Vektor3f mx;
        };
        std::vector<Kasten3> liste;
        Befund block = neu("blockgitter", Schwere::Hinweis, "q3map2 facebsp.cpp:59-62 (nodes split at every _blocksize boundary, bsp.cpp:260)");
        block.schaetzung = true;
        const int bs = o.blockGroesse;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            if (!weltArtig(ents[e])) { continue; }
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch || pr.entartet || pr.flaechen.size() < 4) { continue; }
                bool sichtbar = false;
                bool deckt = true;
                for (const AbbildFlaeche& f : pr.flaechen) {
                    const ShaderArt& sa = artVon(o, f.shader, vorrat);
                    if (sa.sichtbar) { sichtbar = true; }
                    if (!sa.undurchsichtig) { deckt = false; }
                }
                if (sichtbar && deckt) { liste.push_back({static_cast<int>(e), static_cast<int>(k), pr.mins, pr.maxs}); }
                // Blockgitter: ein Strukturpinsel ragt knapp ueber eine Blockgrenze
                if (!pr.detail && bs > 0 && deckt) {
                    bool knapp = false;
                    for (std::size_t i = 0; i < 3 && !knapp; ++i) {
                        const double mn = pr.mins[i];
                        const double mx = pr.maxs[i];
                        const double grenze = bs * (std::floor(mn / bs) + 1);
                        if (grenze < mx - 0.5) {
                            const double ueber = std::min(mx - grenze, grenze - mn);
                            if (ueber > 0.5 && ueber < 32) { knapp = true; }
                        }
                    }
                    if (knapp) { add(block, ObjektRef{static_cast<int>(e), static_cast<int>(k), -1, stelle(ents[e], static_cast<int>(e), static_cast<int>(k))}); }
                }
            }
        }
        if (block.anzahl > 0) {
            block.detail = std::to_string(block.anzahl) + " structural brushes reach less than 32 units across a " + std::to_string(bs) + " block boundary";
            out.push_back(block);
        }
        // Ueberlappung: Kehrbesen ueber x, dann die Trennebenen beider Pinsel
        std::sort(liste.begin(), liste.end(), [](const Kasten3& x, const Kasten3& y) { return x.mn[0] < y.mn[0]; });
        Befund ueb = neu("ueberlappung", Schwere::Hinweis, "tutorial r142 (overlapping brushes: z-fighting, double faces; miter walls instead)");
        auto trennt = [&](const AbbildPrimitiv& a1, const AbbildPrimitiv& b1) {
            for (const AbbildFlaeche& f : a1.flaechen) {
                double mn = 1e30;
                for (const AbbildFlaeche& g : b1.flaechen) {
                    for (const Vektor3d& v : g.punkte) { mn = std::min(mn, abstandZuPunkt(f.ebene, v)); }
                }
                if (mn >= -1.0) { return true; }
            }
            return false;
        };
        long long paare = 0;
        for (std::size_t i = 0; i < liste.size(); ++i) {
            for (std::size_t j = i + 1; j < liste.size(); ++j) {
                if (liste[j].mn[0] >= liste[i].mx[0] - 1) { break; }
                const Kasten3& A = liste[i];
                const Kasten3& B = liste[j];
                if (B.mx[0] <= A.mn[0] + 1 || B.mn[1] >= A.mx[1] - 1 || B.mx[1] <= A.mn[1] + 1 || B.mn[2] >= A.mx[2] - 1 || B.mx[2] <= A.mn[2] + 1) { continue; }
                const AbbildPrimitiv& pa = ents[static_cast<std::size_t>(A.ent)].prims[static_cast<std::size_t>(A.prim)];
                const AbbildPrimitiv& pb = ents[static_cast<std::size_t>(B.ent)].prims[static_cast<std::size_t>(B.prim)];
                if (trennt(pa, pb) || trennt(pb, pa)) { continue; }
                ++paare;
                if (ueb.objekte.size() + 2 <= kMaxObjekte) {
                    ueb.objekte.push_back(ObjektRef{A.ent, A.prim, -1, stelle(ents[static_cast<std::size_t>(A.ent)], A.ent, A.prim)});
                    ueb.objekte.push_back(ObjektRef{B.ent, B.prim, -1, stelle(ents[static_cast<std::size_t>(B.ent)], B.ent, B.prim)});
                }
            }
        }
        ueb.anzahl = paare;
        if (paare > 0) {
            ueb.detail = std::to_string(paare) + " pairs of opaque brushes overlapping by more than 1 unit";
            out.push_back(ueb);
        }
    }

    messen("ueberlappung");
    // --- 6. T-Verbindungen (q3map2 tjunction.cpp behebt sie mit zusaetzlichen Punkten) -----
    {
        // Ecken einmalig (auf 1/8 gerundet) mit dem ersten Besitzer, Kanten je
        // Pinsel einmalig; je Kante die Ecken in den Rasterzellen ihres Kastens
        struct Ecke {
            Vektor3d p;
            int ent;
            int prim;
            bool zweiBesitzer;   // gehoert auch einem anderen Pinsel
        };
        std::vector<Ecke> ecken;
        std::unordered_map<EbenenSchluessel, int, EbenenHash> eckenIndex;   // (x, y, z, 0) auf 1/8
        struct Kante {
            int a;
            int b;
            int ent;
            int prim;
        };
        std::vector<Kante> kanten;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            if (!weltArtig(ents[e])) { continue; }
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch || pr.entartet) { continue; }
                std::set<std::pair<int, int>> eigene;
                for (const AbbildFlaeche& f : pr.flaechen) {
                    if (!artVon(o, f.shader, vorrat).sichtbar || f.punkte.size() < 3) { continue; }
                    std::vector<int> idx;
                    idx.reserve(f.punkte.size());
                    for (const Vektor3d& v : f.punkte) {
                        const EbenenSchluessel schl{std::llround(v[0] * 8), std::llround(v[1] * 8), std::llround(v[2] * 8), 0};
                        auto it = eckenIndex.find(schl);
                        if (it == eckenIndex.end()) {
                            it = eckenIndex.emplace(schl, static_cast<int>(ecken.size())).first;
                            ecken.push_back({v, static_cast<int>(e), static_cast<int>(k), false});
                        } else {
                            Ecke& ek = ecken[static_cast<std::size_t>(it->second)];
                            if (ek.ent != static_cast<int>(e) || ek.prim != static_cast<int>(k)) { ek.zweiBesitzer = true; }
                        }
                        idx.push_back(it->second);
                    }
                    for (std::size_t i = 0; i < idx.size(); ++i) {
                        const int a1 = idx[i];
                        const int b1 = idx[(i + 1) % idx.size()];
                        if (a1 == b1) { continue; }
                        if (eigene.insert({std::min(a1, b1), std::max(a1, b1)}).second) { kanten.push_back({a1, b1, static_cast<int>(e), static_cast<int>(k)}); }
                    }
                }
            }
        }
        constexpr double kZelle = 64.0;
        auto zelle = [](double v) { return static_cast<long long>(std::floor(v / kZelle)); };
        std::unordered_map<EbenenSchluessel, std::vector<int>, EbenenHash> raster;
        for (std::size_t i = 0; i < ecken.size(); ++i) {
            const Vektor3d& p = ecken[i].p;
            raster[EbenenSchluessel{zelle(p[0]), zelle(p[1]), zelle(p[2]), 0}].push_back(static_cast<int>(i));
        }
        Befund b = neu("tjunction", Schwere::Hinweis, "q3map2 tjunction.cpp FixTJunctions (adds vertices; unfixed T-junctions sparkle)");
        b.schaetzung = true;
        std::vector<char> markiert(ecken.size(), 0);
        for (const Kante& k : kanten) {
            const Vektor3d& pa = ecken[static_cast<std::size_t>(k.a)].p;
            const Vektor3d& pb = ecken[static_cast<std::size_t>(k.b)].p;
            const Vektor3d d = pb - pa;
            const double l2 = laengeQuadrat(d);
            if (l2 < 1.0) { continue; }
            const long long x0 = zelle(std::min(pa[0], pb[0]) - 0.1);
            const long long x1 = zelle(std::max(pa[0], pb[0]) + 0.1);
            const long long y0 = zelle(std::min(pa[1], pb[1]) - 0.1);
            const long long y1 = zelle(std::max(pa[1], pb[1]) + 0.1);
            const long long z0 = zelle(std::min(pa[2], pb[2]) - 0.1);
            const long long z1 = zelle(std::max(pa[2], pb[2]) + 0.1);
            if ((x1 - x0 + 1) * (y1 - y0 + 1) * (z1 - z0 + 1) > 4096) { continue; }   // riesige schraege Kanten uebergehen
            for (long long x = x0; x <= x1; ++x) {
                for (long long y = y0; y <= y1; ++y) {
                    for (long long z = z0; z <= z1; ++z) {
                        const auto it = raster.find(EbenenSchluessel{x, y, z, 0});
                        if (it == raster.end()) { continue; }
                        for (const int vi : it->second) {
                            if (vi == k.a || vi == k.b || markiert[static_cast<std::size_t>(vi)] != 0) { continue; }
                            const Ecke& ek = ecken[static_cast<std::size_t>(vi)];
                            if (ek.ent == k.ent && ek.prim == k.prim && !ek.zweiBesitzer) { continue; }
                            const double t = punkt(ek.p - pa, d) / l2;
                            if (t <= 0.0 || t >= 1.0) { continue; }
                            const Vektor3d naechster = pa + d * t;
                            if (laengeQuadrat(ek.p - naechster) > 0.01) { continue; }
                            if (laengeQuadrat(ek.p - pa) < 0.01 || laengeQuadrat(ek.p - pb) < 0.01) { continue; }
                            markiert[static_cast<std::size_t>(vi)] = 1;
                            add(b, ObjektRef{ek.ent, ek.prim, -1, stelle(ents[static_cast<std::size_t>(ek.ent)], ek.ent, ek.prim)});
                        }
                    }
                }
            }
        }
        if (b.anzahl > 0) {
            b.detail = std::to_string(b.anzahl) + " brush vertices lie inside an edge of another brush (T-junctions)";
            out.push_back(b);
        }
    }

    messen("tjunction");
    // --- 7. Patches: Kollisionsebenen, Facetten, Gitter; Clip-Modelle -----------------------
    {
        const Grenze* ge = grenzeSuchen(o.spiel, "kollision.patchebenen");
        const Grenze* gf = grenzeSuchen(o.spiel, "kollision.patchfacetten");
        Befund b = neu("patchgross", Schwere::Warnung, "OpenJK code/qcommon/cm_patch.cpp CM_FindPlane / CM_PatchCollideFromGrid (MAX_PATCH_PLANES, MAX_FACETS: Com_Error)");
        b.behebung = Behebung::PatchTeilen;
        Befund gitter = neu("patchgitter", Schwere::Fehler, "OpenJK code/qcommon/cm_patch.cpp CM_SubdivideGridColumns writes into cGrid_t::points[129][129] without a check");
        gitter.behebung = Behebung::PatchTeilen;
        long long schlimmste = 0;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (!pr.patch) { continue; }
                const PatchKollision pk = patchKollision(pr.breite, pr.hoehe, pr.kontrollpunkte);
                if (!pk.gueltig) { continue; }
                const ObjektRef r{static_cast<int>(e), static_cast<int>(k), -1,
                                  stelle(ents[e], static_cast<int>(e), static_cast<int>(k)) + " " + std::to_string(pk.ebenen) + " planes, " + std::to_string(pk.facetten) + " facets"};
                const bool ebeneKnapp = ge != nullptr && pk.ebenen >= o.knappAb * static_cast<double>(ge->erlaubt);
                const bool facettenKnapp = gf != nullptr && pk.facetten >= o.knappAb * static_cast<double>(gf->erlaubt);
                if (ebeneKnapp || facettenKnapp) {
                    add(b, r);
                    if ((ge != nullptr && pk.ebenen > ge->erlaubt) || (gf != nullptr && pk.facetten > gf->erlaubt)) { b.schwere = Schwere::Fehler; }
                    schlimmste = std::max<long long>(schlimmste, pk.ebenen);
                }
                if (pk.groessteGitterSeite > 129) { add(gitter, r); }
            }
        }
        if (b.anzahl > 0) {
            b.detail = std::to_string(b.anzahl) + " patches near or over the collision limits (most planes: " + std::to_string(schlimmste) + ")";
            out.push_back(b);
        }
        if (gitter.anzahl > 0) {
            gitter.detail = std::to_string(gitter.anzahl) + " patches subdivide to more than 129 grid columns";
            out.push_back(gitter);
        }
        Befund clip = neu("clipmodell", Schwere::Hinweis, "q3map2 model.cpp:359 eClipModel (spawnflags 2): a clip brush per triangle; tutorial: use physics_clip brushes");
        for (std::size_t e = 0; e < ents.size(); ++e) {
            if (ents[e].klasse != "misc_model") { continue; }
            const int sf = ganz(ents[e].wert("spawnflags"));
            constexpr int kClip = (1 << 1) | (1 << 3) | (1 << 4) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12);   // eClipFlags
            if ((sf & kClip) != 0) { add(clip, ObjektRef{static_cast<int>(e), -1, -1, stelle(ents[e], static_cast<int>(e), -1) + " " + ents[e].wert("model")}); }
        }
        if (clip.anzahl > 0) {
            clip.detail = std::to_string(clip.anzahl) + " misc_model with clip flags";
            out.push_back(clip);
        }
    }

    messen("patches");
    // --- 8. Fehlende Texturen ----------------------------------------------------------------
    if (o.shaderBekanntGeprueft) {
        Befund b = neu("textur", Schwere::Warnung, "q3map2 shaders.cpp (\"Couldn't find image for shader\"), engine R_FindShader (default shader)");
        std::map<std::string, ObjektRef> fehlen;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                auto pruefen = [&](const std::string& s) {
                    const std::string kl = klein(s);
                    if (kl.empty() || kl == "textures/" || beginntMit(kl, "textures/common/") || beginntMit(kl, "textures/system/")) { return; }
                    if (!artVon(o, s, vorrat).bekannt) {
                        fehlen.emplace(kl, ObjektRef{static_cast<int>(e), static_cast<int>(k), -1, s});
                    }
                };
                if (pr.patch) { pruefen(pr.shader); }
                for (const AbbildFlaeche& f : pr.flaechen) { pruefen(f.shader); }
            }
        }
        for (const auto& [k, r] : fehlen) { add(b, r); }
        if (b.anzahl > 0) {
            b.detail = std::to_string(b.anzahl) + " shaders without a shader script or image";
            out.push_back(b);
        }
    }

    messen("textur");
    // --- 9. Brush-Entities ohne Pinsel (SV_SetBrushModel: NULL) ------------------------------
    {
        Befund b = neu("leereentity", Schwere::Fehler, "OpenJK code/server/sv_game.cpp SV_SetBrushModel (\"SV_SetBrushModel: NULL\"); q3map2 map.cpp (origin brushes are removed)");
        b.behebung = Behebung::EntitaetEntfernen;
        for (std::size_t e = 1; e < ents.size(); ++e) {
            const AbbildEntitaet& en = ents[e];
            if (!en.behaelter || weltArtig(en) || en.klasse == "worldspawn") { continue; }
            // nur Ursprungspinsel (common/origin) zaehlen nicht
            bool echt = false;
            for (const AbbildPrimitiv& pr : en.prims) {
                if (pr.patch) {
                    echt = true;
                    break;
                }
                bool nurUrsprung = !pr.flaechen.empty();
                for (const AbbildFlaeche& f : pr.flaechen) {
                    if (klein(f.shader) != "textures/common/origin" && klein(f.shader) != "textures/system/origin") { nurUrsprung = false; }
                }
                if (!nurUrsprung) {
                    echt = true;
                    break;
                }
            }
            if (!echt && !en.hat("model")) { add(b, ObjektRef{static_cast<int>(e), -1, -1, stelle(en, static_cast<int>(e), -1)}); }
        }
        if (b.anzahl > 0) {
            b.detail = std::to_string(b.anzahl) + " brush entities without brushes or patches";
            out.push_back(b);
        }
    }

    messen("leereentity");
    // --- 10. Was nur die gebaute BSP zeigt --------------------------------------------------
    if (o.bsp != nullptr && o.bsp->ok) {
        const BspAnalyse& an = *o.bsp;
        if (an.lichtgitterUeberlauf > 0 || an.lichtgitterFehlpassung) {
            Befund b = neu("lichtgitter", Schwere::Fehler,
                           an.lichtgitterUeberlauf > 0 ? "q3map2 bspfile_rbsp.cpp:158 (index 65535 when the grid is full), OpenJK tr_bsp.cpp R_LoadLightGridArray / tr_light.cpp"
                                                       : "OpenJK code/rd-vanilla/tr_bsp.cpp R_LoadLightGridArray \"light grid array mismatch\" (entities unlit)");
            b.behebung = Behebung::WeltSchluessel;
            b.schluessel = "gridsize";
            char z[96];
            std::snprintf(z, sizeof(z), "%.0f %.0f %.0f", an.gitterGroesse[0] * 2, an.gitterGroesse[1] * 2, an.gitterGroesse[2]);
            b.wert = z;
            b.anzahl = an.lichtgitterUeberlauf;
            b.detail = an.lichtgitterUeberlauf > 0 ? std::to_string(an.lichtgitterUeberlauf) + " light grid cells point behind the light grid (index 65535)"
                                                   : "light grid array has " + std::to_string(an.lichtgitterErwartet) + " cells expected by the engine";
            out.push_back(b);
        }
        // Die Zahl der Lightmaps (Renderer, hart) vor der Groesse des Lumps
        // (MAX_MAP_LIGHTING, Formatgrenze): die Anzahl des Befunds sind dann Lightmaps
        std::vector<const Bewertung*> licht;
        for (const char* id : {"renderer.lightmaps", "bsp.licht"}) {
            for (const Bewertung& bw : o.bewertungen) {
                if (bw.zaehlung.id == id) { licht.push_back(&bw); }
            }
        }
        for (const Bewertung* lb : licht) {
            const Bewertung& bw = *lb;
            if (bw.zaehlung.quelle != ZaehlQuelle::Bsp || (bw.ampel != Ampel::Knapp && bw.ampel != Ampel::Ueber && bw.ampel != Ampel::Hinweis)) { continue; }
            {
                // _lightmapscale (lightmaps_ydnar.cpp: Lightmap-Massstab je Entity/worldspawn)
                const std::string jetzt = ents.empty() ? std::string() : ents.front().wert("_lightmapscale");
                const double alt = jetzt.empty() ? 1.0 : std::atof(jetzt.c_str());
                Befund b = neu("lightmapscale", bw.ampel == Ampel::Ueber ? Schwere::Fehler : Schwere::Warnung,
                               "q3map2 lightmaps_ydnar.cpp (_lightmapscale), tutorial r142 (\"MAX_MAP_LIGHTING\", too many lightmaps)");
                b.behebung = Behebung::WeltSchluessel;
                b.schluessel = "_lightmapscale";
                char z[32];
                std::snprintf(z, sizeof(z), "%g", alt * 2.0);
                b.wert = z;
                b.anzahl = bw.zaehlung.wert;
                b.detail = bw.zaehlung.id + " = " + std::to_string(bw.zaehlung.wert) + " (" + std::to_string(static_cast<int>(bw.anteil * 100)) + " %)";
                out.push_back(b);
                break;
            }
        }
        if (an.portale > 0) {
            // VIS-Aufwand (vis.cpp PassagePortalFlow): mit den Portalen waechst er schnell
            const long long schwelle = o.portalSchwelle;
            if (an.portale >= schwelle) {
                Befund b = neu("vis", Schwere::Hinweis, "q3map2 vis.cpp / visflow.cpp (PassagePortalFlow), tutorial r147 \"Reducing passage portal flow\": detail, hint, _blocksize");
                b.schaetzung = true;
                b.anzahl = an.portale;
                b.detail = std::to_string(an.portale) + " portals, " + std::to_string(an.cluster) + " clusters";
                out.push_back(b);
            }
        }
        if (an.dreieckeJeAnsicht > 0) {
            const long long schwelle = o.dreieckSchwelle;   // Faustregel der Tutorials (r_speeds)
            if (an.dreieckeJeAnsicht >= schwelle) {
                Befund b = neu("dreiecke", Schwere::Hinweis, "OpenJK tr_main.cpp (r_speeds), tutorials (RichDiesal: keep the triangles per view low) - estimate from the PVS");
                b.schaetzung = true;
                b.anzahl = an.dreieckeJeAnsicht;
                b.detail = std::to_string(an.dreieckeJeAnsicht) + " world triangles in the PVS of cluster " + std::to_string(an.dreieckeCluster);
                out.push_back(b);
            }
        }
        if (o.bspDaten != nullptr) {
            Befund b = neu("spawnimfesten", Schwere::Fehler, "OpenJK code/qcommon/cm_test.cpp CM_PointLeafnum (cluster -1: in solid or outside); tutorial r142 (spawn point in the void)");
            for (std::size_t e = 0; e < ents.size(); ++e) {
                const std::string& k = ents[e].klasse;
                if (k != "info_player_start" && k != "info_player_deathmatch" && !beginntMit(k, "NPC_")) { continue; }
                float p[3] = {0, 0, 0};
                if (std::sscanf(ents[e].wert("origin").c_str(), "%f %f %f", &p[0], &p[1], &p[2]) != 3) { continue; }
                // ein wenig ueber dem Ursprung (der Ursprung liegt oft genau auf dem Boden)
                const int c = bspCluster(*o.bspDaten, Vektor3f(p[0], p[1], p[2] + 8));
                if (c == -1) { add(b, ObjektRef{static_cast<int>(e), -1, -1, stelle(ents[e], static_cast<int>(e), -1) + " at " + ents[e].wert("origin")}); }
            }
            if (b.anzahl > 0) {
                b.detail = std::to_string(b.anzahl) + " spawn points or NPC spawners in solid space or outside the map";
                out.push_back(b);
            }
            // misc_model_static: gezeichnet wird nur, wenn der Pruefpunkt
            // origin + (0 0 zoffset+1) in einem Cluster liegt (OpenJK#1358)
            Befund m = neu("modellunsichtbar", Schwere::Fehler,
                           "OpenJK code/game/g_misc.cpp:417 (zoffset), MovieDuels code/cgame/cg_main.cpp:3226-3229 CG_DrawMiscEnts (cull point origin + zoffset + 1), "
                           "OpenJK code/server/sv_game.cpp:236-239 SV_inPVS (cluster -1: mask[-1], never drawn); OpenJK#1358");
            m.behebung = Behebung::EntitaetSchluessel;
            m.schluessel = "zoffset";
            long long ohneWert = 0;
            for (std::size_t e = 0; e < ents.size(); ++e) {
                if (klein(ents[e].klasse) != "misc_model_static") { continue; }
                float p[3] = {0, 0, 0};
                (void)std::sscanf(ents[e].wert("origin").c_str(), "%f %f %f", &p[0], &p[1], &p[2]);
                const float zoff = static_cast<float>(std::atof(ents[e].wert("zoffset").c_str()));
                if (bspCluster(*o.bspDaten, Vektor3f(p[0], p[1], p[2] + zoff + 1.0F)) != -1) { continue; }
                ObjektRef r{static_cast<int>(e), -1, -1, stelle(ents[e], static_cast<int>(e), -1) + " at " + ents[e].wert("origin")};
                // der kleinste Versatz (1 Einheit genau, bis 1024), der den Pruefpunkt aus dem Festen holt
                for (int d = 1; d <= 1024 && r.wert.empty(); ++d) {
                    for (const int richtung : {1, -1}) {
                        const float versuch = zoff + static_cast<float>(richtung * d);
                        if (bspCluster(*o.bspDaten, Vektor3f(p[0], p[1], p[2] + versuch + 1.0F)) != -1) {
                            char z[32];
                            std::snprintf(z, sizeof(z), "%g", static_cast<double>(versuch));
                            r.wert = z;
                            break;
                        }
                    }
                }
                if (r.wert.empty()) { ++ohneWert; }
                add(m, r);
            }
            if (m.anzahl > 0) {
                m.detail = std::to_string(m.anzahl) + " misc_model_static whose cull point lies in solid space (the game never draws them)" +
                           (ohneWert > 0 ? ", " + std::to_string(ohneWert) + " without a zoffset within 1024 units - move them" : std::string());
                out.push_back(m);
            }
        }
    }
    messen("bsp");
    // --- 11. Brush-Modelle zusammenlegen (wenn MAX_SUBMODELS knapp wird) -------------------
    for (const Bewertung& bw : o.bewertungen) {
        if (bw.zaehlung.id != "kollision.submodelle" || (bw.ampel != Ampel::Knapp && bw.ampel != Ampel::Ueber)) { continue; }
        // Gruppen gleicher Klasse und gleicher Schluessel, ohne targetname/target
        std::map<std::string, std::vector<int>> gruppen;
        for (std::size_t e = 1; e < ents.size(); ++e) {
            const AbbildEntitaet& en = ents[e];
            if (en.prims.empty() || weltArtig(en) || en.hat("targetname") || en.hat("target") || en.hat("script_targetname")) { continue; }
            std::vector<std::pair<std::string, std::string>> s;
            for (const auto& kv : en.schluessel) {
                const std::string k = klein(kv.first);
                if (k == "model") { continue; }   // "origin" bleibt: der Drehpunkt muss gleich sein
                s.emplace_back(k, kv.second);
            }
            std::sort(s.begin(), s.end());
            std::string schl;
            for (const auto& kv : s) { schl += kv.first + "=" + kv.second + ";"; }
            gruppen[schl].push_back(static_cast<int>(e));
        }
        Befund b = neu("modellezusammen", bw.ampel == Ampel::Ueber ? Schwere::Fehler : Schwere::Warnung,
                       "OpenJK code/qcommon/cm_load.cpp:143 (MAX_SUBMODELS); brush entities with the same keys can be one model");
        b.behebung = Behebung::ModelleZusammenlegen;
        for (const auto& [schl, liste] : gruppen) {
            if (liste.size() < 2) { continue; }
            for (const int e : liste) { add(b, ObjektRef{e, -1, -1, stelle(ents[static_cast<std::size_t>(e)], e, -1)}); }
        }
        if (b.anzahl > 0) {
            b.detail = std::to_string(b.anzahl) + " brush entities in groups with identical keys (no targetname)";
            out.push_back(b);
        }
        break;
    }
    messen("modelle");
    // --- 12. aus bekannten Fehlern: common/origin, riesige Pinsel, Flaechenlicht-Backsplash ----
    {
        Befund co = neu("commonorigin", Schwere::Fehler,
                        "NRC#270; q3map2 map.cpp:792 (only a shader with surfaceparm origin, games.cpp:803 C_ORIGIN, makes an origin brush - "
                        "Jedi Academy and Jedi Outcast define textures/system/origin): the entity ends up at 0 0 0");
        co.behebung = Behebung::TexturErsetzen;
        co.wert = "textures/system/origin";
        Befund rp = neu("riesenpinsel", Schwere::Hinweis, "GtkRadiant#271 (huge brushes such as a sky box leak: plane precision); q3map2 common/maxworld.h MAX_WORLD_COORD");
        Befund fl = neu("flaechenlichtleck", Schwere::Warnung,
                        "q3map2 light_bounce.cpp:551-575 (area backsplash light: the light surface moved by q3map_backsplash distance, default 23, "
                        "q3map2.h:100-101); GL#189, NRC#144 (light through brushes above lava)");
        for (std::size_t e = 0; e < ents.size(); ++e) {
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch || pr.entartet) { continue; }
                const ObjektRef r{static_cast<int>(e), static_cast<int>(k), -1, stelle(ents[e], static_cast<int>(e), static_cast<int>(k))};
                double groesste = 0;
                for (int i = 0; i < 3; ++i) { groesste = std::max(groesste, static_cast<double>(pr.maxs[i] - pr.mins[i])); }
                if (groesste >= o.riesigAb) { add(rp, r); }
                for (std::size_t i = 0; i < pr.flaechen.size(); ++i) {
                    const AbbildFlaeche& f = pr.flaechen[i];
                    const std::string sh = klein(f.shader);
                    const ShaderArt& art = artVon(o, f.shader, vorrat);
                    if (sh == "textures/common/origin" && !art.ursprung) {
                        ObjektRef rf = r;
                        rf.flaeche = static_cast<int>(i);
                        rf.text += ", face " + std::to_string(i);
                        add(co, rf);
                    }
                    if (art.flaechenlicht <= 0 || art.backsplashAnteil <= 0 || art.himmel || f.punkte.size() < 3) { continue; }
                    // die Backsplash-Flaeche liegt backsplashWeite vor der Lichtflaeche: liegt
                    // zwischen beiden ein fester Pinsel, leuchtet sie auf seiner anderen Seite
                    Vektor3d mitte(0, 0, 0);
                    for (const Vektor3d& v : f.punkte) { mitte = mitte + v; }
                    mitte = mitte * (1.0 / static_cast<double>(f.punkte.size()));
                    const Vektor3d n = f.ebene.normal();
                    std::vector<Vektor3d> proben;
                    for (double d = 1.0; d <= art.backsplashWeite + 1e-9; d += 1.0) { proben.push_back(mitte + n * d); }
                    for (const Vektor3d& v : f.punkte) { proben.push_back(v + n * art.backsplashWeite); }
                    // erst der Kasten um alle Proben, dann nur Pinsel, die ihn schneiden
                    double pmin[3] = {proben.front()[0], proben.front()[1], proben.front()[2]};
                    double pmax[3] = {pmin[0], pmin[1], pmin[2]};
                    for (const Vektor3d& v : proben) {
                        for (int a2 = 0; a2 < 3; ++a2) {
                            pmin[a2] = std::min(pmin[a2], v[a2]);
                            pmax[a2] = std::max(pmax[a2], v[a2]);
                        }
                    }
                    bool leck = false;
                    for (std::size_t e2 = 0; e2 < ents.size() && !leck; ++e2) {
                        for (std::size_t k2 = 0; k2 < ents[e2].prims.size() && !leck; ++k2) {
                            if (e2 == e && k2 == k) { continue; }
                            const AbbildPrimitiv& q = ents[e2].prims[k2];
                            if (q.patch || q.entartet || q.flaechen.empty()) { continue; }
                            if (pmax[0] < q.mins[0] || pmin[0] > q.maxs[0] || pmax[1] < q.mins[1] || pmin[1] > q.maxs[1] || pmax[2] < q.mins[2] ||
                                pmin[2] > q.maxs[2]) {
                                continue;
                            }
                            bool fest = true;
                            for (const AbbildFlaeche& qf : q.flaechen) {
                                const ShaderArt& qa = artVon(o, qf.shader, vorrat);
                                if (!qa.undurchsichtig || qa.werkzeug) { fest = false; }
                            }
                            if (!fest) { continue; }
                            for (const Vektor3d& v : proben) {
                                if (v[0] < q.mins[0] || v[0] > q.maxs[0] || v[1] < q.mins[1] || v[1] > q.maxs[1] || v[2] < q.mins[2] || v[2] > q.maxs[2]) { continue; }
                                if (imPinsel(q, v, 0.01)) {
                                    leck = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (leck) {
                        ObjektRef rf = r;
                        rf.flaeche = static_cast<int>(i);
                        rf.text += ", face " + std::to_string(i) + " " + f.shader;
                        add(fl, rf);
                    }
                }
            }
        }
        if (co.anzahl > 0) {
            co.detail = std::to_string(co.anzahl) + " faces with textures/common/origin (no surfaceparm origin in this game)";
            out.push_back(co);
        }
        if (rp.anzahl > 0) {
            rp.detail = std::to_string(rp.anzahl) + " brushes with an edge of " + std::to_string(static_cast<long long>(o.riesigAb)) + " units or more";
            out.push_back(rp);
        }
        if (fl.anzahl > 0) {
            fl.detail = std::to_string(fl.anzahl) + " surface light faces whose backsplash lies behind a solid brush";
            out.push_back(fl);
        }
    }
    // --- 13. Schalter im Build-Menue, die mit diesem Spiel nicht gehen ------------------
    {
        Befund bb = neu("baubefehl", Schwere::Fehler,
                        "q3map2 light.cpp:2559-2584 (-external, -lightmapsize: external lightmaps), lightmaps_ydnar.cpp:3109; NRC#98 (Jedi Outcast), NRC#48 (light styles)");
        for (const BauBefehlProblem& p : bauBefehlProbleme(o.spiel, lichtstile(a), o.bauBefehle)) {
            add(bb, ObjektRef{-1, -1, -1, p.eintrag + ": " + p.schalter + " - " + p.grund});
        }
        if (bb.anzahl > 0) {
            bb.detail = std::to_string(bb.anzahl) + " build menu commands with switches this game cannot use (the build menu refuses them)";
            out.push_back(bb);
        }
    }
    messen("bekannt");
    return out;
}

// Die Woerter einer .shader-Datei wie q3map2s Skript-Leser (scriplib.cpp
// GetToken: // und /* */ sind Kommentar, "..." ein Wort, { } eigene Woerter).
std::vector<std::string> shaderWoerter(const std::string& t) {
    std::vector<std::string> w;
    std::size_t i = 0;
    const std::size_t n = t.size();
    while (i < n) {
        const char c = t[i];
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            ++i;
        } else if (c == '/' && i + 1 < n && t[i + 1] == '/') {
            while (i < n && t[i] != '\n') { ++i; }
        } else if (c == '/' && i + 1 < n && t[i + 1] == '*') {
            const std::size_t e = t.find("*/", i + 2);
            i = e == std::string::npos ? n : e + 2;
        } else if (c == '{' || c == '}') {
            w.emplace_back(1, c);
            ++i;
        } else if (c == '"') {
            const std::size_t e = t.find('"', i + 1);
            w.push_back(t.substr(i + 1, (e == std::string::npos ? n : e) - i - 1));
            i = e == std::string::npos ? n : e + 1;
        } else {
            const std::size_t a = i;
            while (i < n && std::isspace(static_cast<unsigned char>(t[i])) == 0 && t[i] != '{' && t[i] != '}') { ++i; }
            w.push_back(t.substr(a, i - a));
        }
    }
    return w;
}

ShaderSkript shaderSkriptLesen(const std::string& text, const std::string& name) {
    ShaderSkript r;
    const std::vector<std::string> w = shaderWoerter(text);
    const std::string gesucht = klein(name);
    int tiefe = 0;
    for (std::size_t i = 0; i < w.size(); ++i) {
        if (w[i] == "{") {
            ++tiefe;
            continue;
        }
        if (w[i] == "}") {
            tiefe = std::max(0, tiefe - 1);
            continue;
        }
        if (tiefe != 0 || klein(w[i]) != gesucht || i + 1 >= w.size() || w[i + 1] != "{") { continue; }
        // der Block dieses Shaders (shaders.cpp ParseShaderFile: nur die oberste Ebene)
        r.gefunden = true;
        int t = 0;
        for (std::size_t j = i + 1; j < w.size(); ++j) {
            if (w[j] == "{") {
                ++t;
                continue;
            }
            if (w[j] == "}") {
                if (--t == 0) { break; }
                continue;
            }
            if (t != 1) { continue; }
            const std::string k = klein(w[j]);
            if (k == "q3map_surfacelight" && j + 1 < w.size()) {   // shaders.cpp: si.value = atof( token )
                r.flaechenlicht = std::atof(w[j + 1].c_str());
            } else if (k == "q3map_backsplash" && j + 2 < w.size()) {   // shaders.cpp:1248-1252
                r.backsplashAnteil = std::atof(w[j + 1].c_str()) * 0.01;
                r.backsplashWeite = std::atof(w[j + 2].c_str());
            } else if (k == "surfaceparm" && j + 1 < w.size()) {
                const std::string parm = klein(w[j + 1]);
                if (parm == "sky") { r.himmel = true; }
                if (parm == "origin") { r.ursprung = true; }
            }
        }
        return r;
    }
    return r;
}

namespace {
bool lichtMitStil(const std::string& klasse, const std::string& stil1, const std::string& stil2, const std::string& name) {
    if (klein(klasse).rfind("light", 0) != 0) { return false; }   // q3map2 light.cpp: classname_prefixed( "light" )
    return std::atoi(stil1.c_str()) > 0 || std::atoi(stil2.c_str()) > 0 || !name.empty();
}
}  // namespace

bool lichtstile(const KartenAbbild& a) {
    for (const AbbildEntitaet& e : a.ents) {
        if (lichtMitStil(e.klasse, e.wert("style"), e.wert("_style"), e.wert("targetname"))) { return true; }
    }
    return false;
}

bool lichtstileInKarte(const Karte& k) {
    for (const auto& e : k.entitaeten()) {
        if (lichtMitStil(e->klassenname(), e->wert("style"), e->wert("_style"), e->wert("targetname"))) { return true; }
    }
    return false;
}

std::vector<BauBefehlProblem> bauBefehlProbleme(GrenzSpiel spiel, bool mitLichtstilen, const std::vector<std::pair<std::string, std::string>>& befehle) {
    std::vector<BauBefehlProblem> r;
    for (const auto& [eintrag, zeile] : befehle) {
        std::vector<std::string> w;
        std::istringstream in(zeile);
        for (std::string x; in >> x;) { w.push_back(x); }
        if (std::find(w.begin(), w.end(), "-light") == w.end()) { continue; }   // -external gehoert zu LIGHT
        std::vector<std::string> schalter;
        for (std::size_t i = 0; i < w.size(); ++i) {
            if (w[i] == "-external") { schalter.emplace_back("-external"); }
            if (w[i] == "-lightmapsize" && i + 1 < w.size()) {
                // light.cpp:2565-2584: jede andere Groesse als die des Spiels (128) speichert extern
                const int b = std::atoi(w[i + 1].c_str());
                const bool zwei = i + 2 < w.size() && std::isdigit(static_cast<unsigned char>(w[i + 2][0])) != 0;
                const int h = zwei ? std::atoi(w[i + 2].c_str()) : b;
                if (b != 128 || h != 128) { schalter.push_back("-lightmapsize " + w[i + 1] + (zwei ? " " + w[i + 2] : std::string())); }
            }
        }
        for (const std::string& sch : schalter) {
            if (spiel == GrenzSpiel::Jk2) {
                r.push_back({eintrag, sch, "Jedi Outcast cannot load external lightmaps (NRC#98); q3map2 light.cpp:2559-2584 stores every lightmap externally"});
            } else if (mitLichtstilen) {
                r.push_back({eintrag, sch,
                             "external lightmaps with light styles are broken in RBSP: styled surfaces lose their lightmap stages (q3map2 lightmaps_ydnar.cpp:3109, NRC#48)"});
            }
        }
    }
    return r;
}

}  // namespace bhed::karte
