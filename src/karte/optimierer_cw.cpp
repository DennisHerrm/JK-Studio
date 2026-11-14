// optimierer_cw.cpp - Karten-Editor, Optimierer: die Pruefungen der Clone-Wars-Kette (CW-PRUEF).
//
// Herkunft: die Anforderungsliste eines Clone-Wars-Mods fuer Jedi Academy / Movie Duels (plaene/clonewars_kette.md):
// Sicht gegen Kollision, fast deckungsgleiche Flaechen, Stufen, Tuer- und Gangmasse, Texturen, Modelle, Requisiten.
// Portiert aus OpenJK und q3map2 (NetRadiant-Custom 20260114): die Masse der Spielfigur, die Grenzen und die Regeln des Compilers
// (Inhalte der Pinsel, Clip-Modelle, entfernte Flaechen) kommen aus deren Quelltext, die Stellen stehen bei jeder Pruefung:
//   OpenJK code/game/bg_public.h:51-53 (DEFAULT_MINS_2, DEFAULT_MAXS_2, CROUCH_MAXS_2), :69 MIN_WALK_NORMAL, :70 JUMP_VELOCITY,
//   :71 STEPSIZE, :716 MASK_PLAYERSOLID; code/game/g_client.cpp:55-58 (DEFAULT_MINS_0 ... DEFAULT_MAXS_1 = -16 / 16),
//   codemp/game/g_client.c:31 (playerMins -15); code/game/g_main.cpp:628 (g_gravity 800);
//   code/rd-vanilla/tr_image.cpp:1002-1004 (R_CreateImage: ERR_FATAL bei Bildseiten, die keine Zweierpotenz sind),
//   rd-common/tr_image_load.cpp:94-145 (R_LoadImage), tr_model.cpp:899-906 (R_LoadMD3), tr_surface.cpp:53-69
//   (RB_CheckOverflow), code/qcommon/qfiles.h:33-34, :101-105; Movie Duels code/cgame/cg_main.cpp:3263-3290
//   (CG_DrawMiscEnts);
//   q3map2 (NetRadiant-Custom 20260114): games.cpp:787-848 (surfaceparm-Inhalte game_sof2), map.cpp:455-503
//   SetBrushContents, writebsp.cpp:376 (die Inhalte des Pinsels in der BSP), model.cpp:970-999 clipflags_doClip,
//   surface.cpp:1150-1360 (SideInBrush, CullSides), portals.cpp:529-599 FloodEntities.
// Die Masse der Liste (1 Einheit, 2 Einheiten, Tuer 110 x 64, Gang 128 x 96, Bilder 64 ... 2048) sind Vorgaben der
// Einstellungen (CwMasse).
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

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace bhed::karte {

Spielfigur spielfigur(GrenzSpiel s) {
    Spielfigur f;
    if (s == GrenzSpiel::JkaMp) { f.halb = 15; }   // codemp/game/g_client.c:31 playerMins = {-15, -15, DEFAULT_MINS_2}
    return f;
}

bool bildKopfMass(const std::string& b, int& breite, int& hoehe) {
    const auto u8 = [&b](std::size_t i) { return static_cast<unsigned>(static_cast<unsigned char>(b[i])); };
    if (b.size() >= 24 && u8(0) == 0x89 && b.compare(1, 3, "PNG") == 0) {   // PNG: IHDR, Breite und Hoehe big-endian
        breite = static_cast<int>((u8(16) << 24) | (u8(17) << 16) | (u8(18) << 8) | u8(19));
        hoehe = static_cast<int>((u8(20) << 24) | (u8(21) << 16) | (u8(22) << 8) | u8(23));
        return breite > 0 && hoehe > 0;
    }
    if (b.size() >= 4 && u8(0) == 0xFF && u8(1) == 0xD8) {   // JPEG: das erste SOFn
        std::size_t i = 2;
        while (i + 9 < b.size()) {
            if (u8(i) != 0xFF) {
                ++i;
                continue;
            }
            const unsigned m = u8(i + 1);
            if (m == 0xFF) {
                ++i;
                continue;
            }
            if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) {
                i += 2;
                continue;
            }
            if (m == 0xD9 || m == 0xDA) { return false; }
            const std::size_t laenge = (u8(i + 2) << 8) | u8(i + 3);
            if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
                hoehe = static_cast<int>((u8(i + 5) << 8) | u8(i + 6));
                breite = static_cast<int>((u8(i + 7) << 8) | u8(i + 8));
                return breite > 0 && hoehe > 0;
            }
            i += 2 + laenge;
        }
        return false;
    }
    if (b.size() >= 18) {   // TGA (ohne Kennung): Bildart 1, 2, 3, 9, 10, 11; Breite und Hoehe little-endian
        const unsigned art = u8(2);
        if (art == 1 || art == 2 || art == 3 || art == 9 || art == 10 || art == 11) {
            breite = static_cast<int>(u8(12) | (u8(13) << 8));
            hoehe = static_cast<int>(u8(14) | (u8(15) << 8));
            return breite > 0 && hoehe > 0;
        }
    }
    return false;
}

namespace {

using V = Vektor3d;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string zahl(double v) {
    char z[32];
    std::snprintf(z, sizeof(z), "%.1f", v);
    std::string s = z;
    if (s.size() > 2 && s.compare(s.size() - 2, 2, ".0") == 0) { s.resize(s.size() - 2); }
    return s;
}
std::string ort(const V& p) { return "(" + zahl(p[0]) + " " + zahl(p[1]) + " " + zahl(p[2]) + ")"; }
V nachV(const Vektor3f& v) { return V(v[0], v[1], v[2]); }

// --- Die Welt fuer Strahlen: Pinsel (konvex) und Dreiecke (Patches, Modelle) in einem Gitter ueber x/y ----------------

struct Vol {
    const AbbildPrimitiv* p = nullptr;
    int ent = -1;
    int prim = -1;
    bool kollision = false;   // der Inhalts-Shader (SetBrushContents) hindert die Spielfigur
    bool gezeichnet = false;  // mindestens eine Seite wird gezeichnet
    bool verdeckend = false;  // gezeichnet und jede Seite undurchsichtig: verdeckt, was in ihm liegt
    bool struktur = true;
    bool durchscheinend = false;   // C_TRANSLUCENT an einer Seite (SideInBrush laesst darin liegende Seiten stehen)
    bool nodrawSeite = false;      // eine Seite mit C_NODRAW (caulk): SideInBrush laesst darin liegende Seiten stehen
    bool mover = false;            // Tuer, Aufzug, Zug, ... (bewegt sich im Spiel: fuer die Masse der Durchgaenge offen)
    int inhalt = 0;                // die Inhalte des Inhalts-Shaders (S_CONT_*)
    double mn[3] = {0, 0, 0};
    double mx[3] = {0, 0, 0};
};
struct Dr {   // ein Dreieck
    Vektor3f a, b, c;
    int ent = -1;
    int prim = -1;
    int modell = -1;   // Index in PruefOptionen::modelle, -1: Patch
    bool sichtbar = false;
    bool kollision = false;
    bool fest = false;   // undurchsichtig (kein Glas, kein Alpha-Test): eine Flaeche, auf der man stehen koennte
};

enum class Modus { Kollision, Sichtbar };

struct Treffer {
    bool ja = false;
    bool innen = false;   // der Anfang liegt in einem Kollisionspinsel
    double t = 0;
    double austritt = 0;  // innen: wo der Strahl den Pinsel verlaesst
    V n{0, 0, 0};
    int vol = -1;
    int dr = -1;
};

class Welt {
public:
    std::vector<Vol> vol;
    std::vector<Dr> dr;
    bool ohneMover = false;   // Strahlen gehen durch Mover (Durchgaenge: Tueren stehen offen)

    void ordnen() {
        m_volStempel.assign(vol.size(), 0);
        m_drStempel.assign(dr.size(), 0);
        for (std::size_t i = 0; i < vol.size(); ++i) { eintragen(static_cast<int>(i), vol[i].mn, vol[i].mx, true); }
        for (std::size_t i = 0; i < dr.size(); ++i) {
            const Dr& d = dr[i];
            const double mn[3] = {std::min({d.a[0], d.b[0], d.c[0]}) * 1.0, std::min({d.a[1], d.b[1], d.c[1]}) * 1.0, 0};
            const double mx[3] = {std::max({d.a[0], d.b[0], d.c[0]}) * 1.0, std::max({d.a[1], d.b[1], d.c[1]}) * 1.0, 0};
            eintragen(static_cast<int>(i), mn, mx, false);
        }
    }

    // Liegt p in Pinsel v (alle Seiten mit Abstand < -eps)?
    static bool innen(const Vol& v, const V& p, double eps) {
        if (p[0] < v.mn[0] - eps || p[0] > v.mx[0] + eps || p[1] < v.mn[1] - eps || p[1] > v.mx[1] + eps || p[2] < v.mn[2] - eps ||
            p[2] > v.mx[2] + eps) {
            return false;
        }
        for (const AbbildFlaeche& f : v.p->flaechen) {
            if (abstandZuPunkt(f.ebene, p) > -eps) { return false; }
        }
        return true;
    }

    // Die Oberkante des konvexen Pinsels ueber p (p liegt darin): wo ein Strahl nach oben ihn verlaesst
    static double obenAus(const Vol& v, const V& p, double* normaleZ = nullptr) {
        double t = 1e30;
        for (const AbbildFlaeche& f : v.p->flaechen) {
            const double nz = f.ebene.normal()[2];
            if (nz > 1e-9) {
                const double tf = -abstandZuPunkt(f.ebene, p) / nz;
                if (tf < t) {
                    t = tf;
                    if (normaleZ != nullptr) { *normaleZ = nz; }
                }
            }
        }
        return p[2] + t;
    }

    // jeder Pinsel, der p enthaelt (erst gesammelt: f darf weitere Abfragen stellen)
    template<class F>
    void enthaltend(const V& p, double eps, F f) const {
        ++m_lauf;
        std::vector<const Vol*> treffer;
        const auto pruefe = [&](int i) {
            if (m_volStempel[static_cast<std::size_t>(i)] == m_lauf) { return; }
            m_volStempel[static_cast<std::size_t>(i)] = m_lauf;
            const Vol& v = vol[static_cast<std::size_t>(i)];
            if (innen(v, p, eps)) { treffer.push_back(&v); }
        };
        const auto it = m_volZellen.find(schluessel(zelle(p[0]), zelle(p[1])));
        if (it != m_volZellen.end()) {
            for (const int i : it->second) { pruefe(i); }
        }
        for (const int i : m_volGross) { pruefe(i); }
        for (const Vol* v : treffer) { f(*v); }
    }

    // Strahl o + t d (d normiert), 0 <= t <= laenge: der naechste Treffer
    [[nodiscard]] Treffer strahl(const V& o, const V& d, double laenge, Modus m, const std::function<const ShaderArt&(const std::string&)>& art) const {
        Treffer best;
        best.t = laenge;
        ++m_lauf;
        const auto pruefeVol = [&](int i) {
            if (m_volStempel[static_cast<std::size_t>(i)] == m_lauf) { return; }
            m_volStempel[static_cast<std::size_t>(i)] = m_lauf;
            const Vol& v = vol[static_cast<std::size_t>(i)];
            if (m == Modus::Kollision ? !v.kollision : !v.gezeichnet) { return; }
            if (ohneMover && v.mover) { return; }
            double tein = -1e30;
            double taus = 1e30;
            int seite = -1;
            for (std::size_t s = 0; s < v.p->flaechen.size(); ++s) {
                const AbbildFlaeche& f = v.p->flaechen[s];
                const V n = f.ebene.normal();
                const double dist = abstandZuPunkt(f.ebene, o);
                const double den = punkt(n, d);
                if (std::fabs(den) < 1e-12) {
                    if (dist > 1e-6) { return; }
                    continue;
                }
                const double t = -dist / den;
                if (den < 0) {
                    if (t > tein) {
                        tein = t;
                        seite = static_cast<int>(s);
                    }
                } else if (t < taus) {
                    taus = t;
                }
                if (tein > taus) { return; }
            }
            if (taus < 0 || tein > taus) { return; }
            if (tein < 0) {   // der Anfang liegt im Pinsel
                if (m == Modus::Kollision && !best.innen) {
                    best = Treffer{};
                    best.ja = true;
                    best.innen = true;
                    best.t = 0;
                    best.austritt = taus;
                    best.vol = i;
                }
                return;
            }
            if (best.innen || tein > best.t) { return; }
            if (m == Modus::Sichtbar) {
                const ShaderArt& a = art(v.p->flaechen[static_cast<std::size_t>(seite)].shader);
                if (!a.sichtbar || a.werkzeug || a.himmel) { return; }
            }
            best.ja = true;
            best.t = tein;
            best.n = v.p->flaechen[static_cast<std::size_t>(seite)].ebene.normal();
            best.vol = i;
            best.dr = -1;
        };
        const auto pruefeDr = [&](int i) {
            if (m_drStempel[static_cast<std::size_t>(i)] == m_lauf) { return; }
            m_drStempel[static_cast<std::size_t>(i)] = m_lauf;
            const Dr& x = dr[static_cast<std::size_t>(i)];
            if (m == Modus::Kollision ? !x.kollision : !x.sichtbar) { return; }
            if (best.innen) { return; }
            // Moeller-Trumbore, beidseitig
            const V a = nachV(x.a);
            const V e1 = nachV(x.b) - a;
            const V e2 = nachV(x.c) - a;
            const V pv = kreuz(d, e2);
            const double det = punkt(e1, pv);
            if (std::fabs(det) < 1e-12) { return; }
            const double inv = 1.0 / det;
            const V tv = o - a;
            const double u = punkt(tv, pv) * inv;
            if (u < -1e-9 || u > 1 + 1e-9) { return; }
            const V qv = kreuz(tv, e1);
            const double w = punkt(d, qv) * inv;
            if (w < -1e-9 || u + w > 1 + 1e-9) { return; }
            const double t = punkt(e2, qv) * inv;
            if (t < 0 || t > best.t) { return; }
            best.ja = true;
            best.t = t;
            V n = kreuz(e1, e2);
            const double l = std::sqrt(laengeQuadrat(n));
            if (l > 0) { n = n * (1.0 / l); }
            if (punkt(n, d) > 0) { n = n * -1.0; }   // zum Anfang hin
            best.n = n;
            best.vol = -1;
            best.dr = i;
        };
        const V e = o + d * laenge;
        const long long x0 = zelle(std::min(o[0], e[0]));
        const long long x1 = zelle(std::max(o[0], e[0]));
        const long long y0 = zelle(std::min(o[1], e[1]));
        const long long y1 = zelle(std::max(o[1], e[1]));
        for (long long x = x0; x <= x1; ++x) {
            for (long long y = y0; y <= y1; ++y) {
                const long long k = schluessel(x, y);
                const auto iv = m_volZellen.find(k);
                if (iv != m_volZellen.end()) {
                    for (const int i : iv->second) { pruefeVol(i); }
                }
                const auto id = m_drZellen.find(k);
                if (id != m_drZellen.end()) {
                    for (const int i : id->second) { pruefeDr(i); }
                }
            }
        }
        for (const int i : m_volGross) { pruefeVol(i); }
        for (const int i : m_drGross) { pruefeDr(i); }
        if (!best.ja) { best.t = 0; }
        return best;
    }

    // jeder Pinsel und jedes Dreieck, deren Kasten den Kasten [mn, mx] (x/y) beruehrt
    template<class FV, class FD>
    void imKasten(const double* mn, const double* mx, FV fv, FD fd) const {
        ++m_lauf;
        const auto pv = [&](int i) {
            if (m_volStempel[static_cast<std::size_t>(i)] == m_lauf) { return; }
            m_volStempel[static_cast<std::size_t>(i)] = m_lauf;
            fv(vol[static_cast<std::size_t>(i)]);
        };
        const auto pd = [&](int i) {
            if (m_drStempel[static_cast<std::size_t>(i)] == m_lauf) { return; }
            m_drStempel[static_cast<std::size_t>(i)] = m_lauf;
            fd(dr[static_cast<std::size_t>(i)], i);
        };
        const long long x0 = zelle(mn[0]);
        const long long x1 = zelle(mx[0]);
        const long long y0 = zelle(mn[1]);
        const long long y1 = zelle(mx[1]);
        if ((x1 - x0 + 1) * (y1 - y0 + 1) > 4096) {   // sehr grosse Kaesten: alles
            for (std::size_t i = 0; i < vol.size(); ++i) { pv(static_cast<int>(i)); }
            for (std::size_t i = 0; i < dr.size(); ++i) { pd(static_cast<int>(i)); }
            return;
        }
        for (long long x = x0; x <= x1; ++x) {
            for (long long y = y0; y <= y1; ++y) {
                const long long k = schluessel(x, y);
                const auto iv = m_volZellen.find(k);
                if (iv != m_volZellen.end()) {
                    for (const int i : iv->second) { pv(i); }
                }
                const auto id = m_drZellen.find(k);
                if (id != m_drZellen.end()) {
                    for (const int i : id->second) { pd(i); }
                }
            }
        }
        for (const int i : m_volGross) { pv(i); }
        for (const int i : m_drGross) { pd(i); }
    }

private:
    static constexpr double kZelle = 128.0;
    static long long zelle(double v) { return static_cast<long long>(std::floor(v / kZelle)); }
    static long long schluessel(long long x, long long y) { return (x << 32) ^ (y & 0xFFFFFFFFLL); }
    void eintragen(int i, const double* mn, const double* mx, bool istVol) {
        const long long x0 = zelle(mn[0]);
        const long long x1 = zelle(mx[0]);
        const long long y0 = zelle(mn[1]);
        const long long y1 = zelle(mx[1]);
        if ((x1 - x0 + 1) * (y1 - y0 + 1) > 1024) {   // riesige Pinsel (Himmelskasten): in jeder Abfrage
            (istVol ? m_volGross : m_drGross).push_back(i);
            return;
        }
        auto& z = istVol ? m_volZellen : m_drZellen;
        for (long long x = x0; x <= x1; ++x) {
            for (long long y = y0; y <= y1; ++y) { z[schluessel(x, y)].push_back(i); }
        }
    }
    std::unordered_map<long long, std::vector<int>> m_volZellen;
    std::unordered_map<long long, std::vector<int>> m_drZellen;
    std::vector<int> m_volGross;
    std::vector<int> m_drGross;
    mutable std::vector<unsigned> m_volStempel;
    mutable std::vector<unsigned> m_drStempel;
    mutable unsigned m_lauf = 0;
};

// --- Hilfen: Messpunkte auf einem ebenen Vieleck, Ueberdeckung zweier Vielecke ----------------------------------------

struct Basis {
    V u;
    V v;
};
Basis basisFuer(const V& n) {
    const double ax = std::fabs(n[0]);
    const double ay = std::fabs(n[1]);
    const double az = std::fabs(n[2]);
    const V achse = (ax <= ay && ax <= az) ? V(1, 0, 0) : (ay <= az ? V(0, 1, 0) : V(0, 0, 1));
    const V u = normiert(kreuz(n, achse));
    return {u, kreuz(n, u)};
}
struct P2 {
    double x;
    double y;
};
std::vector<P2> flach(const std::vector<V>& pts, const V& ursprung, const Basis& b) {
    std::vector<P2> r;
    r.reserve(pts.size());
    for (const V& p : pts) { r.push_back({punkt(p - ursprung, b.u), punkt(p - ursprung, b.v)}); }
    return r;
}
double flaeche2(const std::vector<P2>& p) {
    double s = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const P2& a = p[i];
        const P2& c = p[(i + 1) % p.size()];
        s += a.x * c.y - c.x * a.y;
    }
    return s * 0.5;
}
// Abstand von q zum Rand des konvexen Vielecks (positiv innen)
double randAbstand(const std::vector<P2>& g, double orient, const P2& q) {
    double best = 1e30;
    for (std::size_t i = 0; i < g.size(); ++i) {
        const P2& a = g[i];
        const P2& c = g[(i + 1) % g.size()];
        const double ex = c.x - a.x;
        const double ey = c.y - a.y;
        const double l = std::sqrt(ex * ex + ey * ey);
        if (l < 1e-9) { continue; }
        best = std::min(best, orient * (ex * (q.y - a.y) - ey * (q.x - a.x)) / l);
    }
    return best;
}
// Messpunkte im Raster (Abstand mindestens `abstand`, hoechstens `hoechstens` Punkte), 0,5 vom Rand; sonst die Mitte
std::vector<V> proben(const std::vector<V>& pts, const V& n, double abstand, std::size_t hoechstens) {
    std::vector<V> r;
    if (pts.size() < 3) { return r; }
    const Basis b = basisFuer(n);
    const std::vector<P2> g = flach(pts, pts[0], b);
    const double f = flaeche2(g);
    const double orient = f >= 0 ? 1.0 : -1.0;
    double mnx = 1e30;
    double mny = 1e30;
    double mxx = -1e30;
    double mxy = -1e30;
    for (const P2& q : g) {
        mnx = std::min(mnx, q.x);
        mny = std::min(mny, q.y);
        mxx = std::max(mxx, q.x);
        mxy = std::max(mxy, q.y);
    }
    const double s = std::max(abstand, std::sqrt(std::fabs(f) / static_cast<double>(std::max<std::size_t>(1, hoechstens))));
    for (double x = mnx + s * 0.5; x < mxx; x += s) {
        for (double y = mny + s * 0.5; y < mxy; y += s) {
            if (randAbstand(g, orient, P2{x, y}) >= 0.5) { r.push_back(pts[0] + b.u * x + b.v * y); }
        }
    }
    if (r.empty()) {
        V m(0, 0, 0);
        for (const V& p : pts) { m = m + p; }
        r.push_back(m * (1.0 / static_cast<double>(pts.size())));
    }
    return r;
}
// Sutherland-Hodgman: Schnitt zweier konvexer Vielecke (beide gegen den Uhrzeigersinn)
std::vector<P2> schnitt(std::vector<P2> a, const std::vector<P2>& b) {
    for (std::size_t i = 0; i < b.size() && !a.empty(); ++i) {
        const P2 p = b[i];
        const P2 q = b[(i + 1) % b.size()];
        const auto seite = [&](const P2& x) { return (q.x - p.x) * (x.y - p.y) - (q.y - p.y) * (x.x - p.x); };
        std::vector<P2> r;
        for (std::size_t j = 0; j < a.size(); ++j) {
            const P2 c = a[j];
            const P2 d = a[(j + 1) % a.size()];
            const double sc = seite(c);
            const double sd = seite(d);
            if (sc >= 0) { r.push_back(c); }
            if ((sc >= 0) != (sd >= 0)) {
                const double t = sc / (sc - sd);
                r.push_back(P2{c.x + (d.x - c.x) * t, c.y + (d.y - c.y) * t});
            }
        }
        a = std::move(r);
    }
    return a;
}
void gegenUhr(std::vector<P2>& p) {
    if (flaeche2(p) < 0) { std::reverse(p.begin(), p.end()); }
}

// q3map2 model.cpp:970-999 clipflags_doClip: erzeugt q3map2 aus den Dreiecken eines misc_model Clip-Pinsel?
bool clipFein(const ShaderArt& si, int spawnFlags) {
    constexpr int eClipModel = 1 << 1;
    constexpr int eExtrudeFaceNormals = 1 << 3;
    constexpr int eExtrudeTerrain = 1 << 4;
    constexpr int eExtrudeVertexNormals = 1 << 7;
    constexpr int ePyramidalClip = 1 << 8;
    constexpr int eExtrudeDownwards = 1 << 9;
    constexpr int eExtrudeUpwards = 1 << 10;
    constexpr int eMaxExtrude = 1 << 11;
    constexpr int eAxialBackplane = 1 << 12;
    constexpr int eClipFlags = eClipModel | eExtrudeFaceNormals | eExtrudeTerrain | eExtrudeVertexNormals | ePyramidalClip | eExtrudeDownwards |
                               eExtrudeUpwards | eMaxExtrude | eAxialBackplane;
    const int spf = spawnFlags & (eClipFlags & ~eClipModel);
    const bool fein = (si.clipModell && spf == 0) || (spawnFlags & eClipFlags) == eClipModel || spf == ePyramidalClip ||
                      spf == (ePyramidalClip | eAxialBackplane) || spf == eExtrudeFaceNormals || spf == (eExtrudeFaceNormals | ePyramidalClip) ||
                      spf == eExtrudeTerrain || spf == eExtrudeDownwards || spf == eExtrudeUpwards || spf == (eExtrudeDownwards | eExtrudeUpwards) ||
                      spf == eAxialBackplane || spf == (eAxialBackplane | eExtrudeTerrain) || spf == (eAxialBackplane | eExtrudeDownwards) ||
                      spf == (eAxialBackplane | eExtrudeUpwards) || spf == (eAxialBackplane | eExtrudeDownwards | eExtrudeUpwards) ||
                      spf == (eMaxExtrude | eExtrudeTerrain) || spf == (eMaxExtrude | eExtrudeDownwards) || spf == (eMaxExtrude | eExtrudeUpwards) ||
                      spf == (eMaxExtrude | eExtrudeDownwards | eExtrudeUpwards) || spf == eExtrudeVertexNormals ||
                      spf == (eExtrudeVertexNormals | ePyramidalClip);
    return (si.cSolid || si.clipModell) && fein;
}

// q3map2 map.cpp:455-503 SetBrushContents: der Shader, dessen Inhalte der Pinsel in der BSP traegt (writebsp.cpp:376)
const ShaderArt& inhaltsShader(const AbbildPrimitiv& p, const std::function<const ShaderArt&(const std::string&)>& art) {
    const ShaderArt* cs = &art(p.flaechen.front().shader);
    for (std::size_t i = 1; i < p.flaechen.size(); ++i) {
        const ShaderArt& s = art(p.flaechen[i].shader);
        if (cs->cFluessig) { continue; }
        if (s.cFluessig) {
            cs = &s;
            continue;
        }
        if (cs->cNebel) { continue; }
        if (s.cNebel) {
            cs = &s;
            continue;
        }
        if ((cs->inhalt & 0x10) != 0) { continue; }   // playerclip (GetRequiredSurfaceParm<"playerclip">().contentFlags)
        if ((s.inhalt & 0x10) != 0) {
            cs = &s;
            continue;
        }
        if (!cs->cSolid) { continue; }
        if (!s.cSolid) { cs = &s; }
    }
    return *cs;
}

// Patch: das Bezier-Netz (3 x 3 je Teilstueck) als Dreiecke, 4 Schritte je Teilstueck
void patchDreiecke(const AbbildPrimitiv& p, const std::function<void(const V&, const V&, const V&)>& f) {
    const int w = p.breite;
    const int h = p.hoehe;
    if (w < 3 || h < 3 || (w % 2) == 0 || (h % 2) == 0 || static_cast<int>(p.kontrollpunkte.size()) < w * h) { return; }
    constexpr int kSchritte = 4;
    const auto kp = [&](int x, int y) {
        const auto& q = p.kontrollpunkte[static_cast<std::size_t>(y * w + x)];
        return V(q[0], q[1], q[2]);
    };
    for (int py = 0; py + 2 < h; py += 2) {
        for (int px = 0; px + 2 < w; px += 2) {
            V g[kSchritte + 1][kSchritte + 1];
            for (int j = 0; j <= kSchritte; ++j) {
                const double v = static_cast<double>(j) / kSchritte;
                const double bv[3] = {(1 - v) * (1 - v), 2 * v * (1 - v), v * v};
                for (int i = 0; i <= kSchritte; ++i) {
                    const double u = static_cast<double>(i) / kSchritte;
                    const double bu[3] = {(1 - u) * (1 - u), 2 * u * (1 - u), u * u};
                    V s(0, 0, 0);
                    for (int b = 0; b < 3; ++b) {
                        for (int a = 0; a < 3; ++a) { s = s + kp(px + a, py + b) * (bu[a] * bv[b]); }
                    }
                    g[j][i] = s;
                }
            }
            for (int j = 0; j < kSchritte; ++j) {
                for (int i = 0; i < kSchritte; ++i) {
                    f(g[j][i], g[j][i + 1], g[j + 1][i + 1]);
                    f(g[j][i], g[j + 1][i + 1], g[j + 1][i]);
                }
            }
        }
    }
}

// --- Die Pruefungen ------------------------------------------------------------------------------------------------

class Cw {
public:
    Cw(const KartenAbbild& a, const PruefOptionen& o, std::vector<Befund>& out, const std::function<const ShaderArt&(const std::string&)>& art)
        : m_a(a), m_o(o), m_out(out), m_art(art), m_fig(spielfigur(o.spiel)), m_hatBsp(o.bspDaten != nullptr && o.linDaten.empty()) {}

    // CW-IMPORT: nur die Liste der Shader und Bilder (Schalter aus)
    void nurInventar() { texturen(); }

    void alle() {
        auto uhr = std::chrono::steady_clock::now();
        const auto messen = [&](const char* was) {
            const auto jetzt = std::chrono::steady_clock::now();
            if (m_o.zeiten != nullptr) { m_o.zeiten->emplace_back(was, std::chrono::duration<double, std::milli>(jetzt - uhr).count()); }
            uhr = jetzt;
        };
        weltBauen();
        knotenBauen();
        messen("cw_welt");
        sichtGegenKollision();
        messen("cw_sicht");
        fastDeckungsgleich();
        messen("cw_deckung");
        stufen();
        messen("cw_stufen");
        durchgaenge();
        messen("cw_masse");
        texturen();
        messen("cw_texturen");
        modelle();
        messen("cw_modelle");
        requisiten();
        messen("cw_requisiten");
    }

private:
    const KartenAbbild& m_a;
    const PruefOptionen& m_o;
    std::vector<Befund>& m_out;
    const std::function<const ShaderArt&(const std::string&)>& m_art;
    Spielfigur m_fig;
    bool m_hatBsp;
    Welt m_w;
    std::vector<std::array<double, 6>> m_modellKasten;   // je Modell der genaue Kasten der Dreiecke (mn, mx); mn > mx: keiner

    static Befund neu(const char* id, Schwere s, const char* quelle) {
        Befund b;
        b.pruefung = id;
        b.schwere = s;
        b.quelle = quelle;
        return b;
    }
    static void add(Befund& b, ObjektRef r) {
        ++b.anzahl;
        if (b.objekte.size() < 2000000) { b.objekte.push_back(std::move(r)); }
    }
    static void punktSetzen(ObjektRef& r, const V& p) {
        r.hatPunkt = true;
        r.punkt = {static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2])};
    }
    std::string stelle(int ent, int prim) const {
        const std::string k = ent >= 0 && ent < static_cast<int>(m_a.ents.size()) ? m_a.ents[static_cast<std::size_t>(ent)].klasse : std::string("?");
        return k + " #" + std::to_string(ent) + (prim >= 0 ? ", primitive " + std::to_string(prim) : std::string());
    }
    ObjektRef flaechenRef(const Vol& v, int seite) const {
        ObjektRef r{v.ent, v.prim, -1, stelle(v.ent, v.prim) + ", face " + std::to_string(seite) + " " + v.p->flaechen[static_cast<std::size_t>(seite)].shader,
                    seite};
        return r;
    }
    ObjektRef dreieckRef(const Dr& d) const {
        if (d.modell >= 0) {
            const AbbildModell& m = m_o.modelle[static_cast<std::size_t>(d.modell)];
            return ObjektRef{m.entitaet, -1, -1, stelle(m.entitaet, -1) + " " + m.pfad};
        }
        return ObjektRef{d.ent, d.prim, -1, stelle(d.ent, d.prim) + " (patch)"};
    }
    // In einem Cluster der .bsp (nicht im Festen, nicht ausserhalb)? Ohne .bsp: ja.
    bool imCluster(const V& p) const {
        if (!m_hatBsp) { return true; }
        return bspCluster(*m_o.bspDaten, Vektor3f(static_cast<float>(p[0]), static_cast<float>(p[1]), static_cast<float>(p[2]))) >= 0;
    }

    // --- Wohin die Spielfigur kommt: Standplaetze auf begehbarer Kollision, verbunden durch Gehen (Stufen bis STEPSIZE), Springen
    // ohne Macht (bis zur Sprunghoehe) und Fallen; geflutet von den Startpunkten, Spawnern, Teleport-Zielen und Gegenstaenden. Ohne
    // einen davon (Testkarten) gilt jede Stelle als erreichbar, die in einem Cluster der .bsp liegt.
    struct Knoten {
        V p;
        bool erreicht = false;
    };
    std::vector<Knoten> m_kn;
    std::unordered_map<long long, std::vector<int>> m_knGitter;   // Zellen 32 x 32
    bool m_mitErreichbar = false;
    static constexpr double kKnotenZelle = 32.0;
    static long long knZelle(double v) { return static_cast<long long>(std::floor(v / kKnotenZelle)); }
    static long long knSchluessel(long long x, long long y) { return (x << 32) ^ (y & 0xFFFFFFFFLL); }

    void knotenBauen() {
        const auto dazu = [&](const V& p) {
            // Platz fuer die Spielfigur (geduckt): bis zur Kollision darueber mindestens 40
            if (m_w.strahl(p + V(0, 0, 0.1), V(0, 0, 1), m_fig.hoeheGeduckt(), Modus::Kollision, m_art).ja) { return; }
            m_knGitter[knSchluessel(knZelle(p[0]), knZelle(p[1]))].push_back(static_cast<int>(m_kn.size()));
            m_kn.push_back(Knoten{p, false});
        };
        m_w.ohneMover = true;   // Tueren stehen offen
        for (const Vol& v : m_w.vol) {
            if (!v.kollision || v.mover) { continue; }
            for (const AbbildFlaeche& f : v.p->flaechen) {
                const V n = f.ebene.normal();
                if (n[2] < m_fig.gehen || f.punkte.size() < 3) { continue; }
                for (const V& p : proben(f.punkte, n, kKnotenZelle, 4096)) {
                    bool belegt = false;   // eine andere Kollision liegt auf der Flaeche
                    m_w.enthaltend(p + V(0, 0, 0.25), 0.01, [&](const Vol& w) {
                        if (&w != &v && w.kollision && !w.mover) { belegt = true; }
                    });
                    if (!belegt) { dazu(p); }
                }
            }
        }
        for (const Dr& d : m_w.dr) {
            if (!d.kollision) { continue; }
            const V a = nachV(d.a);
            const V b = nachV(d.b);
            const V c = nachV(d.c);
            V n = kreuz(b - a, c - a);
            const double l = std::sqrt(laengeQuadrat(n));
            if (l < 1e-9) { continue; }
            n = n * (1.0 / l);
            if (n[2] < 0) { n = n * -1.0; }
            if (n[2] < m_fig.gehen) { continue; }
            for (const V& p : proben({a, b, c}, n, kKnotenZelle, 64)) { dazu(p); }
        }
        // Saat: Spieler- und Spawn-Punkte, NPC-Spawner, Teleport-Ziele, Gegenstaende
        std::vector<int> offen;
        for (const AbbildEntitaet& en : m_a.ents) {
            const std::string k = klein(en.klasse);
            const bool saat = k.rfind("info_player", 0) == 0 || k.rfind("npc_", 0) == 0 || k == "misc_teleporter_dest" || k == "target_position" ||
                              k.rfind("item_", 0) == 0 || k.rfind("weapon_", 0) == 0 || k.rfind("ammo_", 0) == 0;
            if (!saat) { continue; }
            float o[3] = {0, 0, 0};
            if (std::sscanf(en.wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]) != 3) { continue; }
            // der Boden unter dem Punkt (hoechstens 256 tiefer)
            const Treffer t = m_w.strahl(V(o[0], o[1], o[2] + 1), V(0, 0, -1), 257, Modus::Kollision, m_art);
            const V boden = t.ja && !t.innen ? V(o[0], o[1], o[2] + 1 - t.t) : V(o[0], o[1], o[2]);
            int best = -1;
            double bestAbstand = 64.0 * 64.0;
            knotenBei(boden, 48, [&](int i) {
                const V d = m_kn[static_cast<std::size_t>(i)].p - boden;
                const double a2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
                if (std::fabs(d[2]) <= m_fig.stufe + 1 && a2 < bestAbstand) {
                    bestAbstand = a2;
                    best = i;
                }
            });
            if (best >= 0 && !m_kn[static_cast<std::size_t>(best)].erreicht) {
                m_kn[static_cast<std::size_t>(best)].erreicht = true;
                offen.push_back(best);
            }
        }
        m_mitErreichbar = !offen.empty();
        // fluten: Nachbarn bis 48 waagerecht (zwei Zellen), hinauf bis zur Sprunghoehe, hinab beliebig; der Weg auf Brusthoehe frei
        while (!offen.empty()) {
            const int i = offen.back();
            offen.pop_back();
            const V pi = m_kn[static_cast<std::size_t>(i)].p;
            knotenBei(pi, 48, [&](int j) {
                Knoten& kj = m_kn[static_cast<std::size_t>(j)];
                if (kj.erreicht) { return; }
                const V d = kj.p - pi;
                const double h = std::sqrt(d[0] * d[0] + d[1] * d[1]);
                if (h > 48 || d[2] > m_fig.sprung) { return; }
                const double z = std::max(pi[2], kj.p[2]) + (m_fig.oben + m_fig.unten) * 0.5 - m_fig.unten;   // Mitte der Spielfigur
                if (h > 0.5) {
                    const V von(pi[0], pi[1], z);
                    const V dir(d[0] / h, d[1] / h, 0);
                    if (m_w.strahl(von, dir, h, Modus::Kollision, m_art).ja) { return; }
                }
                kj.erreicht = true;
                offen.push_back(j);
            });
        }
        m_w.ohneMover = false;
    }
    template<class F>
    void knotenBei(const V& p, double radius, F f) const {
        const long long x0 = knZelle(p[0] - radius);
        const long long x1 = knZelle(p[0] + radius);
        const long long y0 = knZelle(p[1] - radius);
        const long long y1 = knZelle(p[1] + radius);
        for (long long x = x0; x <= x1; ++x) {
            for (long long y = y0; y <= y1; ++y) {
                const auto it = m_knGitter.find(knSchluessel(x, y));
                if (it == m_knGitter.end()) { continue; }
                for (const int i : it->second) { f(i); }
            }
        }
    }
    // Die zu Fuss verbundenen Flaechen (Standplaetze, Nachbarn bis 48 waagerecht und hoechstens STEPSIZE hoeher oder tiefer, der Weg
    // frei; nur erreichte Plaetze, ohne Saat alle) und ihr Kasten waagerecht - fuer "Stufe": eine Kiste oder ein Pult hat eine
    // kleine Deckflaeche, die zu Fuss mit nichts verbunden ist.
    std::vector<int> m_knKomp;
    std::vector<std::array<double, 4>> m_kompKasten;   // min x, min y, max x, max y
    bool knRelevant(std::size_t i) const { return !m_mitErreichbar || m_kn[i].erreicht; }
    void komponentenBauen() {
        if (!m_knKomp.empty() || m_kn.empty()) { return; }
        m_knKomp.assign(m_kn.size(), -1);
        m_w.ohneMover = true;
        for (std::size_t s0 = 0; s0 < m_kn.size(); ++s0) {
            if (m_knKomp[s0] >= 0 || !knRelevant(s0)) { continue; }
            const int c = static_cast<int>(m_kompKasten.size());
            std::array<double, 4> kasten{1e30, 1e30, -1e30, -1e30};
            std::vector<int> offen{static_cast<int>(s0)};
            m_knKomp[s0] = c;
            while (!offen.empty()) {
                const int j = offen.back();
                offen.pop_back();
                const V pj = m_kn[static_cast<std::size_t>(j)].p;
                kasten[0] = std::min(kasten[0], pj[0]);
                kasten[1] = std::min(kasten[1], pj[1]);
                kasten[2] = std::max(kasten[2], pj[0]);
                kasten[3] = std::max(kasten[3], pj[1]);
                knotenBei(pj, 48, [&](int k) {
                    if (m_knKomp[static_cast<std::size_t>(k)] >= 0 || !knRelevant(static_cast<std::size_t>(k))) { return; }
                    const V d = m_kn[static_cast<std::size_t>(k)].p - pj;
                    const double h = std::sqrt(d[0] * d[0] + d[1] * d[1]);
                    if (h > 48 || std::fabs(d[2]) > m_fig.stufe) { return; }
                    if (h > 0.5) {
                        const double z = std::max(pj[2], pj[2] + d[2]) + (m_fig.oben + m_fig.unten) * 0.5 - m_fig.unten;
                        if (m_w.strahl(V(pj[0], pj[1], z), V(d[0] / h, d[1] / h, 0), h, Modus::Kollision, m_art).ja) { return; }
                    }
                    m_knKomp[static_cast<std::size_t>(k)] = c;
                    offen.push_back(k);
                });
            }
            m_kompKasten.push_back(kasten);
        }
        m_w.ohneMover = false;
    }
    // Gehoert p zu einer zu Fuss verbundenen Flaeche, die mindestens `mass` x `mass` gross ist (Kasten der Standplaetze plus eine
    // Zelle)?
    bool grosseFlaeche(const V& p, double mass) const {
        int best = -1;
        double bestAbstand = 40.0 * 40.0;
        knotenBei(p, 40, [&](int i) {
            if (m_knKomp[static_cast<std::size_t>(i)] < 0) { return; }
            const V d = m_kn[static_cast<std::size_t>(i)].p - p;
            const double a2 = d[0] * d[0] + d[1] * d[1];
            if (std::fabs(d[2]) <= 2 && a2 <= bestAbstand) {
                bestAbstand = a2;
                best = i;
            }
        });
        if (best < 0) { return false; }
        const auto& k = m_kompKasten[static_cast<std::size_t>(m_knKomp[static_cast<std::size_t>(best)])];
        return k[2] - k[0] + kKnotenZelle >= mass && k[3] - k[1] + kKnotenZelle >= mass;
    }

    // Ein erreichter Standplatz waagerecht hoechstens `radius` entfernt, dessen Hoehe zwischen p.z - unter und p.z + ueber liegt?
    // Ohne Saat: in einem Cluster der .bsp (bzw. ohne .bsp ja).
    bool erreichbarNahe(const V& p, double radius, double unter, double ueber) const {
        if (!m_mitErreichbar) { return imCluster(p + V(0, 0, 1)); }
        bool ja = false;
        knotenBei(p, radius, [&](int i) {
            if (ja) { return; }
            const Knoten& k = m_kn[static_cast<std::size_t>(i)];
            if (!k.erreicht) { return; }
            const double dx = k.p[0] - p[0];
            const double dy = k.p[1] - p[1];
            if (dx * dx + dy * dy <= radius * radius && k.p[2] >= p[2] - unter && k.p[2] <= p[2] + ueber) { ja = true; }
        });
        return ja && imCluster(p + V(0, 0, 1));
    }
    bool gezeichnet(const std::string& shader) const {
        const ShaderArt& a = m_art(shader);
        return a.sichtbar && !a.werkzeug && !a.himmel;
    }
    // Liegt q in einem gezeichneten Pinsel (dann sieht man ihn statt der Flaeche: verdeckt), in einem festen, undurchsichtigen
    // (caulk, nodraw: die Flaeche liegt im Festen) oder in einem unsichtbaren Kollisionspinsel (playerclip, terrain)? Gibt die
    // hoechste Oberkante eines unsichtbaren Kollisionspinsels ueber q zurueck (-1e30: keiner).
    // rampe: die Oberseite dieses Kollisionspinsels ist schraeg (eine Clip-Rampe ueber einer Treppe, wie die Liste sie verlangt)
    double bedeckt(const V& q, bool& verdeckt, bool* rampe = nullptr) const {
        verdeckt = false;
        double oben = -1e30;
        m_w.enthaltend(q, 0.01, [&](const Vol& v) {
            if (v.gezeichnet || (v.inhalt & 0x8001) == 0x8001) {   // gezeichnet bzw. SOLID | OPAQUE
                verdeckt = true;
            } else if (v.kollision) {
                double nz = 1;
                const double z = Welt::obenAus(v, q, &nz);
                if (z > oben) {
                    oben = z;
                    if (rampe != nullptr) { *rampe = nz < 0.99; }
                }
            }
        });
        return oben;
    }

    // --- Welt ---------------------------------------------------------------------------------------------
    void weltBauen() {
        const auto& ents = m_a.ents;
        for (std::size_t e = 0; e < ents.size(); ++e) {
            for (std::size_t k = 0; k < ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = ents[e].prims[k];
                if (pr.patch) {
                    const ShaderArt& sa = m_art(pr.shader);
                    Dr vorlage;
                    vorlage.ent = static_cast<int>(e);
                    vorlage.prim = static_cast<int>(k);
                    vorlage.sichtbar = sa.sichtbar && !sa.werkzeug && !sa.himmel;
                    vorlage.kollision = spielerKollision(sa);
                    vorlage.fest = sa.sichtDeckend && !sa.cFluessig && !sa.cNebel;
                    if (!vorlage.sichtbar && !vorlage.kollision) { continue; }
                    patchDreiecke(pr, [&](const V& a, const V& b, const V& c) {
                        Dr d = vorlage;
                        d.a = Vektor3f(static_cast<float>(a[0]), static_cast<float>(a[1]), static_cast<float>(a[2]));
                        d.b = Vektor3f(static_cast<float>(b[0]), static_cast<float>(b[1]), static_cast<float>(b[2]));
                        d.c = Vektor3f(static_cast<float>(c[0]), static_cast<float>(c[1]), static_cast<float>(c[2]));
                        m_w.dr.push_back(d);
                    });
                    continue;
                }
                if (pr.entartet || pr.flaechen.size() < 4) { continue; }
                Vol v;
                v.p = &pr;
                v.ent = static_cast<int>(e);
                v.prim = static_cast<int>(k);
                v.inhalt = inhaltsShader(pr, m_art).inhalt;
                v.kollision = (v.inhalt & (0x1 | 0x10 | 0x1000)) != 0;   // MASK_PLAYERSOLID
                v.struktur = !pr.detail;
                {
                    const std::string kl = klein(ents[e].klasse);
                    v.mover = kl.rfind("func_door", 0) == 0 || kl == "func_plat" || kl == "func_train" || kl == "func_rotating" || kl == "func_bobbing" ||
                              kl == "func_pendulum";
                }
                bool alleDeckend = true;
                for (const AbbildFlaeche& f : pr.flaechen) {
                    const ShaderArt& sa = m_art(f.shader);
                    if (sa.sichtbar && !sa.werkzeug && !sa.himmel && f.punkte.size() >= 3) { v.gezeichnet = true; }
                    if (!sa.undurchsichtig) { alleDeckend = false; }
                    if (!sa.undurchsichtig) { v.durchscheinend = true; }
                    if (!sa.sichtbar && !sa.werkzeug) { v.nodrawSeite = true; }
                }
                v.verdeckend = v.gezeichnet && alleDeckend;
                for (int i = 0; i < 3; ++i) {
                    v.mn[i] = pr.mins[static_cast<std::size_t>(i)];
                    v.mx[i] = pr.maxs[static_cast<std::size_t>(i)];
                }
                if (v.kollision || v.gezeichnet) { m_w.vol.push_back(v); }
            }
        }
        m_modellKasten.assign(m_o.modelle.size(), {1e30, 1e30, 1e30, -1e30, -1e30, -1e30});
        for (std::size_t i = 0; i < m_o.modelle.size(); ++i) {
            const AbbildModell& m = m_o.modelle[i];
            if (!m.modell || m.modell->null) { continue; }
            auto& kasten = m_modellKasten[i];
            for (std::size_t s = 0; s < m.modell->flaechen.size(); ++s) {
                const ModellFlaeche& f = m.modell->flaechen[s];
                const std::string sh = s < m.shader.size() ? m.shader[s] : f.shader;
                if (sh.empty()) { continue; }   // die Skin bildet die Flaeche nicht ab
                const ShaderArt& sa = m_art(sh);
                Dr vorlage;
                vorlage.ent = m.entitaet;
                vorlage.modell = static_cast<int>(i);
                vorlage.sichtbar = sa.sichtbar && !sa.werkzeug && !sa.himmel;
                vorlage.kollision = m.eingebacken && clipFein(sa, m.spawnflags) && spielerKollision(sa);
                vorlage.fest = sa.sichtDeckend && !sa.cFluessig && !sa.cNebel;
                std::vector<Vektor3f> w;
                w.reserve(f.punkte.size());
                for (const ModellPunkt& p : f.punkte) {
                    const Vektor3f q = transformierterPunkt(m.matrix, p.ort);
                    w.push_back(q);
                    if (vorlage.sichtbar) {
                        for (int a = 0; a < 3; ++a) {
                            kasten[static_cast<std::size_t>(a)] = std::min(kasten[static_cast<std::size_t>(a)], static_cast<double>(q[static_cast<std::size_t>(a)]));
                            kasten[static_cast<std::size_t>(a + 3)] = std::max(kasten[static_cast<std::size_t>(a + 3)], static_cast<double>(q[static_cast<std::size_t>(a)]));
                        }
                    }
                }
                if (!vorlage.sichtbar && !vorlage.kollision) { continue; }
                for (std::size_t t = 0; t + 2 < f.indizes.size(); t += 3) {
                    const std::uint32_t i0 = f.indizes[t];
                    const std::uint32_t i1 = f.indizes[t + 1];
                    const std::uint32_t i2 = f.indizes[t + 2];
                    if (i0 >= w.size() || i1 >= w.size() || i2 >= w.size()) { continue; }
                    Dr d = vorlage;
                    d.a = w[i0];
                    d.b = w[i1];
                    d.c = w[i2];
                    m_w.dr.push_back(d);
                }
            }
        }
        m_w.ordnen();
    }

    // --- 1. Sicht gegen Kollision ---------------------------------------------------------------------------------
    void sichtGegenKollision() {
        static const char* const kQuelle =
            "q3map2 games.cpp:787-848 (surfaceparm contents), map.cpp:455-503 SetBrushContents and writebsp.cpp:376 (the content shader of a "
            "brush), model.cpp:970-999 clipflags_doClip (misc_model clip); OpenJK code/game/bg_public.h:716 MASK_PLAYERSOLID, :69 MIN_WALK_NORMAL, "
            ":51-52 DEFAULT_MINS_2 / DEFAULT_MAXS_2; Clone Wars list: visible floor and collision at most 1 unit apart (adjustable)";
        Befund fall = neu("laufohnekollision", Schwere::Warnung, kQuelle);
        Befund abst = neu("kollisionsabstand", Schwere::Warnung, kQuelle);
        Befund unsicht = neu("unsichtbarekollision", Schwere::Warnung, kQuelle);
        const double tol = m_o.masse.toleranz;
        const double hoehe = m_fig.hoehe();
        double groesster = 0;
        // eine sichtbare, begehbare Flaeche (Wicklung pts, Normale n)
        const auto pruefe = [&](const std::vector<V>& pts, const V& n, ObjektRef ref) {
            if (n[2] < m_fig.gehen) { return; }
            bool faellt = false;
            V fallPunkt;
            double luecke = 0;
            V schlimmst;
            for (const V& p : proben(pts, n, 16, 64)) {
                // die Spielfigur kommt hin: ein erreichter Standplatz daneben, von dem sie hinauf springen oder hinab fallen kann
                if (!erreichbarNahe(p, 48, m_fig.sprung + 1, m_fig.hoehe() * 4)) { continue; }
                bool verdeckt = false;
                bool rampe = false;
                const double oben = bedeckt(p + n * 0.25, verdeckt, &rampe);
                if (verdeckt) { continue; }
                if (oben > -1e29) {   // ein unsichtbarer Kollisionspinsel auf der Flaeche: man steht hoeher
                    const double d = oben - p[2];
                    if (rampe && d <= m_fig.stufe) { continue; }   // Clip-Rampe ueber einer Treppe: hoechstens eine Stufe ueber den Tritten
                    if (d > tol && d <= hoehe && d > luecke) {
                        luecke = d;
                        schlimmst = p;
                    }
                    continue;
                }
                const Treffer t = m_w.strahl(p + V(0, 0, tol), V(0, 0, -1), tol + hoehe, Modus::Kollision, m_art);
                if (!t.ja) {
                    if (!faellt) {
                        faellt = true;
                        fallPunkt = p;
                    }
                    continue;
                }
                if (t.innen) { continue; }
                const double d = t.t - tol;   // so weit liegt die Kollision unter der sichtbaren Flaeche
                if (d > tol && d > luecke) {
                    luecke = d;
                    schlimmst = p;
                }
            }
            if (faellt) {
                ref.text += " at " + ort(fallPunkt);
                punktSetzen(ref, fallPunkt);
                add(fall, ref);
            } else if (luecke > tol) {
                ref.text += ": " + zahl(luecke) + " units at " + ort(schlimmst);
                punktSetzen(ref, schlimmst);
                add(abst, ref);
                groesster = std::max(groesster, luecke);
            }
        };
        for (const Vol& v : m_w.vol) {
            for (std::size_t s = 0; s < v.p->flaechen.size(); ++s) {
                const AbbildFlaeche& f = v.p->flaechen[s];
                const ShaderArt& sa = m_art(f.shader);
                if (!gezeichnet(f.shader) || !sa.sichtDeckend || sa.cFluessig || sa.cNebel || f.punkte.size() < 3) { continue; }
                pruefe(f.punkte, f.ebene.normal(), flaechenRef(v, static_cast<int>(s)));
            }
        }
        // Patches und Modelle: je Teil einmal (die erste Stelle)
        std::set<std::pair<int, int>> gemeldet;
        for (const Dr& d : m_w.dr) {
            if (!d.sichtbar || !d.fest) { continue; }
            const std::pair<int, int> teil = d.modell >= 0 ? std::make_pair(-1, d.modell) : std::make_pair(d.ent, d.prim);
            if (gemeldet.count(teil) != 0) { continue; }
            const V a = nachV(d.a);
            const V b = nachV(d.b);
            const V c = nachV(d.c);
            V n = kreuz(b - a, c - a);
            const double l = std::sqrt(laengeQuadrat(n));
            if (l < 1e-9) { continue; }
            n = n * (1.0 / l);
            // Modelle und Patches sind beidseitig sichtbar: die Seite nach oben
            if (n[2] < 0) { n = n * -1.0; }
            const long long fa = fall.anzahl;
            const long long ab = abst.anzahl;
            pruefe({a, b, c}, n, dreieckRef(d));
            if (fall.anzahl != fa || abst.anzahl != ab) { gemeldet.insert(teil); }
        }
        // Unsichtbare Kollision - nur wo die Spielfigur hinkommt (die Standplaetze; ohne Saat erst mit der .bsp des letzten Baus)
        if (m_hatBsp || m_mitErreichbar) {
            for (const Vol& v : m_w.vol) {
                if (!v.kollision) { continue; }
                for (std::size_t s = 0; s < v.p->flaechen.size(); ++s) {
                    const AbbildFlaeche& f = v.p->flaechen[s];
                    if (f.punkte.size() < 3 || gezeichnet(f.shader)) { continue; }
                    const V n = f.ebene.normal();
                    if (n[2] <= -m_fig.gehen) { continue; }   // Decken
                    const bool boden = n[2] >= m_fig.gehen;
                    for (const V& p : proben(f.punkte, n, 32, 16)) {
                        // Boden: ein erreichter Standplatz darauf; Wand: einer davor, die Stelle hoechstens Koerper + Sprung darueber
                        if (boden ? !erreichbarNahe(p, 24, 1, 1) : !erreichbarNahe(p + n * m_fig.halb, m_fig.halb + 16, m_fig.hoehe() + m_fig.sprung, 0)) {
                            continue;
                        }
                        bool belegt = false;
                        m_w.enthaltend(p + n * 0.25, 0.01, [&](const Vol& w) {
                            if (&w != &v && (w.kollision || w.verdeckend)) { belegt = true; }
                        });
                        if (belegt) { continue; }
                        const Treffer t = boden ? m_w.strahl(p + V(0, 0, tol), V(0, 0, -1), tol + m_fig.stufe, Modus::Sichtbar, m_art)
                                                : m_w.strahl(p + n * tol, n * -1.0, tol + m_fig.halb, Modus::Sichtbar, m_art);
                        if (t.ja) { continue; }
                        ObjektRef r = flaechenRef(v, static_cast<int>(s));
                        r.text += (boden ? " (floor) at " : " (wall) at ") + ort(p);
                        punktSetzen(r, p);
                        add(unsicht, r);
                        break;
                    }
                }
            }
        }
        if (fall.anzahl > 0) {
            detailSetzen(fall, "%s visible walkable faces without collision below them (the player falls through)", {std::to_string(fall.anzahl)});
            m_out.push_back(fall);
        }
        if (unsicht.anzahl > 0) {
            detailSetzen(unsicht, "%s collision faces the player cannot see (invisible floor or wall)", {std::to_string(unsicht.anzahl)});
            m_out.push_back(unsicht);
        }
        if (abst.anzahl > 0) {
            detailSetzen(abst, "%s visible walkable faces more than %s units away from the collision (largest gap %s)",
                         {std::to_string(abst.anzahl), zahl(tol), zahl(groesster)});
            m_out.push_back(abst);
        }
    }

    // --- 2. Fast deckungsgleiche Flaechen (Z-Fighting) ---------------------------------------------------------------
    void fastDeckungsgleich() {
        Befund b = neu("fastdeckungsgleich", Schwere::Warnung,
                       "q3map2 surface.cpp:1197-1360 CullSides (removes only identical windings), :1150-1188 SideInBrush (sides inside "
                       "another brush), ClipSidesIntoTree (structural brushes clip their sides); Clone Wars list: no faces closer than 2 units "
                       "(adjustable)");
        const double grenze = m_o.masse.deckung;
        struct F {
            int vol;
            int seite;
            V n;
            double d;
            double mn[3];
            double mx[3];
        };
        std::vector<F> fl;
        for (std::size_t i = 0; i < m_w.vol.size(); ++i) {
            const Vol& v = m_w.vol[i];
            for (std::size_t s = 0; s < v.p->flaechen.size(); ++s) {
                const AbbildFlaeche& f = v.p->flaechen[s];
                if (f.punkte.size() < 3 || !gezeichnet(f.shader) || m_art(f.shader).versatz) { continue; }
                F x{static_cast<int>(i), static_cast<int>(s), f.ebene.normal(), f.ebene.dist(), {1e30, 1e30, 1e30}, {-1e30, -1e30, -1e30}};
                for (const V& p : f.punkte) {
                    for (int a = 0; a < 3; ++a) {
                        x.mn[a] = std::min(x.mn[a], p[static_cast<std::size_t>(a)]);
                        x.mx[a] = std::max(x.mx[a], p[static_cast<std::size_t>(a)]);
                    }
                }
                fl.push_back(x);
            }
        }
        // je Richtung (auf 1/1000) und Abstandsstreifen der Breite `grenze`
        std::map<std::array<long long, 4>, std::vector<int>> eimer;
        const auto schl = [&](const F& f, long long dv) {
            return std::array<long long, 4>{std::llround(f.n[0] * 1000), std::llround(f.n[1] * 1000), std::llround(f.n[2] * 1000),
                                            static_cast<long long>(std::floor(f.d / std::max(grenze, 0.01))) + dv};
        };
        for (std::size_t i = 0; i < fl.size(); ++i) { eimer[schl(fl[i], 0)].push_back(static_cast<int>(i)); }
        // SideInBrush: liegt die Wicklung auf oder hinter jeder Ebene des Pinsels (der nicht durchscheint und keine caulk-Seite hat)?
        const auto drin = [&](const F& f, const Vol& in) {
            if (in.durchscheinend || in.nodrawSeite) { return false; }
            const AbbildFlaeche& af = m_w.vol[static_cast<std::size_t>(f.vol)].p->flaechen[static_cast<std::size_t>(f.seite)];
            for (const AbbildFlaeche& e : in.p->flaechen) {
                for (const V& p : af.punkte) {
                    if (abstandZuPunkt(e.ebene, p) > 0.1) { return false; }
                }
            }
            return true;
        };
        long long paare = 0;
        double naechster = 1e30;
        for (std::size_t i = 0; i < fl.size(); ++i) {
            const F& f = fl[i];
            for (long long dv = 0; dv <= 1; ++dv) {
                const auto it = eimer.find(schl(f, dv));
                if (it == eimer.end()) { continue; }
                for (const int j : it->second) {
                    if (dv == 0 && j <= static_cast<int>(i)) { continue; }
                    const F& g = fl[static_cast<std::size_t>(j)];
                    if (g.vol == f.vol) { continue; }
                    if (punkt(f.n, g.n) < 0.9999) { continue; }
                    // derselbe Shader zeigt auf beiden dasselbe Bild - das Flackern sieht man nicht
                    if (klein(m_w.vol[static_cast<std::size_t>(f.vol)].p->flaechen[static_cast<std::size_t>(f.seite)].shader) ==
                        klein(m_w.vol[static_cast<std::size_t>(g.vol)].p->flaechen[static_cast<std::size_t>(g.seite)].shader)) {
                        continue;
                    }
                    const double abstand = std::fabs(f.d - g.d);
                    if (abstand >= grenze) { continue; }
                    bool trennt = false;
                    for (int a = 0; a < 3; ++a) {
                        if (f.mn[a] > g.mx[a] + grenze || g.mn[a] > f.mx[a] + grenze) { trennt = true; }
                    }
                    if (trennt) { continue; }
                    const Vol& va = m_w.vol[static_cast<std::size_t>(f.vol)];
                    const Vol& vb = m_w.vol[static_cast<std::size_t>(g.vol)];
                    const AbbildFlaeche& fa = va.p->flaechen[static_cast<std::size_t>(f.seite)];
                    const AbbildFlaeche& fb = vb.p->flaechen[static_cast<std::size_t>(g.seite)];
                    // CullSides: gleiche Ebene und gleiche Wicklung (CULL_EPSILON 0.1) - q3map2 entfernt eine der beiden
                    if (abstand < 0.1 && fa.punkte.size() == fb.punkte.size()) {
                        bool alle = true;
                        for (const V& p : fa.punkte) {
                            bool da = false;
                            for (const V& q : fb.punkte) {
                                if (std::fabs(p[0] - q[0]) <= 0.1 && std::fabs(p[1] - q[1]) <= 0.1 && std::fabs(p[2] - q[2]) <= 0.1) { da = true; }
                            }
                            if (!da) { alle = false; }
                        }
                        if (alle) { continue; }
                    }
                    if (drin(f, vb) || drin(g, va)) { continue; }
                    // die Ueberdeckung in der Ebene
                    const Basis bs = basisFuer(f.n);
                    std::vector<P2> pa = flach(fa.punkte, fa.punkte[0], bs);
                    std::vector<P2> pb = flach(fb.punkte, fa.punkte[0], bs);
                    gegenUhr(pa);
                    gegenUhr(pb);
                    const std::vector<P2> s = schnitt(pa, pb);
                    if (s.size() < 3 || std::fabs(flaeche2(s)) < 1.0) { continue; }
                    // Mitte der Ueberdeckung; liegt sie in einem Strukturpinsel des anderen Teils, schneidet die BSP sie weg
                    double sx = 0;
                    double sy = 0;
                    for (const P2& q : s) {
                        sx += q.x;
                        sy += q.y;
                    }
                    sx /= static_cast<double>(s.size());
                    sy /= static_cast<double>(s.size());
                    const V mitte = fa.punkte[0] + bs.u * sx + bs.v * sy;
                    const V mitteB = mitte + f.n * (g.d - f.d);
                    if ((vb.struktur && vb.verdeckend && Welt::innen(vb, mitte, 0.01)) || (va.struktur && va.verdeckend && Welt::innen(va, mitteB, 0.01))) {
                        continue;
                    }
                    ++paare;
                    naechster = std::min(naechster, abstand);
                    ObjektRef ra = flaechenRef(va, f.seite);
                    ObjektRef rb = flaechenRef(vb, g.seite);
                    ra.text += ": " + zahl(abstand) + " units from " + stelle(vb.ent, vb.prim) + ", at " + ort(mitte);
                    rb.text += ": " + zahl(abstand) + " units from " + stelle(va.ent, va.prim) + ", at " + ort(mitteB);
                    punktSetzen(ra, mitte);
                    punktSetzen(rb, mitteB);
                    if (b.objekte.size() + 2 <= 2000000) {
                        b.objekte.push_back(ra);
                        b.objekte.push_back(rb);
                    }
                }
            }
        }
        b.anzahl = paare;
        if (paare > 0) {
            detailSetzen(b, "%s pairs of drawn faces of different parts face the same way less than %s units apart (z-fighting)",
                         {std::to_string(paare), zahl(grenze)});
            m_out.push_back(b);
        }
    }

    // --- 3. Stufen ueber STEPSIZE -------------------------------------------------------------------------------------
    void stufen() {
        Befund b = neu("stufehoch", Schwere::Warnung,
                       "OpenJK code/game/bg_public.h:71 STEPSIZE 18 (bg_pmove.cpp PM_StepSlideMove steps up at most this high), :70 "
                       "JUMP_VELOCITY 225 and g_gravity 800 (g_main.cpp:628): a riser higher than 225^2 / 1600 = 31.6 is no step but a wall; "
                       "the upper surface belongs to a walkable area of at least 96 x 96 (adjustable) - crates and consoles are obstacles");
        komponentenBauen();
        double hoechste = 0;
        for (const Vol& v : m_w.vol) {
            if (!v.kollision) { continue; }
            for (std::size_t s = 0; s < v.p->flaechen.size(); ++s) {
                const AbbildFlaeche& f = v.p->flaechen[s];
                const V n = f.ebene.normal();
                if (n[2] < m_fig.gehen || f.punkte.size() < 3) { continue; }
                V mitte(0, 0, 0);
                for (const V& p : f.punkte) { mitte = mitte + p; }
                mitte = mitte * (1.0 / static_cast<double>(f.punkte.size()));
                double hier = 0;
                V wo;
                for (std::size_t i = 0; i < f.punkte.size(); ++i) {
                    const V a = f.punkte[i];
                    const V e = f.punkte[(i + 1) % f.punkte.size()] - a;
                    const double lh = std::sqrt(e[0] * e[0] + e[1] * e[1]);
                    if (lh < 1) { continue; }
                    V senk(-e[1] / lh, e[0] / lh, 0);
                    if (punkt(senk, (a + e * 0.5) - mitte) < 0) { senk = senk * -1.0; }
                    const int zahlProben = std::max(1, static_cast<int>(lh / 16.0));
                    // die Flaeche ist die obere Seite einer Stufe: neben ihrer Kante (2 Einheiten hinaus) liegt der Boden tiefer - so
                    // findet sich auch eine Stufe, die mitten auf einer grossen Bodenflaeche steht
                    for (int k = 0; k < zahlProben; ++k) {
                        const V p = a + e * ((k + 0.5) / zahlProben);
                        if (!imCluster(p + V(0, 0, 1))) { continue; }
                        // Platz zum Stehen auf der Stufe
                        const V oben = p - senk * m_fig.halb + V(0, 0, 0.1);
                        if (m_w.strahl(oben, V(0, 0, 1), m_fig.hoehe(), Modus::Kollision, m_art).ja) { continue; }
                        const V q = p + senk * 2.0;
                        const V o(q[0], q[1], p[2] + 0.5);
                        bool wand = false;   // neben der Kante geht es hoch (Wand, hoeherer Boden): keine Stufe hinab
                        m_w.enthaltend(o, 0.01, [&](const Vol& w) {
                            if (w.kollision) { wand = true; }
                        });
                        if (wand) { continue; }
                        const Treffer t = m_w.strahl(o, V(0, 0, -1), m_fig.sprung + 1.0, Modus::Kollision, m_art);
                        if (!t.ja || t.innen || t.n[2] < m_fig.gehen) { continue; }
                        const double boden = o[2] - t.t;
                        const double stufe = p[2] - boden;
                        if (stufe <= m_fig.stufe + 0.1 || stufe > m_fig.sprung) { continue; }
                        // Platz zum Stehen vor der Stufe
                        if (m_w.strahl(V(q[0], q[1], boden + 0.1), V(0, 0, 1), m_fig.hoehe(), Modus::Kollision, m_art).ja) { continue; }
                        if (!erreichbarNahe(V(q[0], q[1], boden), 32, 2, 2) && !erreichbarNahe(p, 32, 2, 2)) { continue; }
                        // oben muss eine zu Fuss verbundene Flaeche von wenigstens dem Mindestmass liegen (keine Kiste, kein Pult)
                        if (!grosseFlaeche(p - senk * 4.0, m_o.masse.stufenFlaeche)) { continue; }
                        if (stufe > hier) {
                            hier = stufe;
                            wo = p;
                        }
                    }
                }
                if (hier > 0) {
                    ObjektRef r = flaechenRef(v, static_cast<int>(s));
                    r.text += ": step " + zahl(hier) + " at " + ort(wo);
                    punktSetzen(r, wo);
                    add(b, r);
                    hoechste = std::max(hoechste, hier);
                }
            }
        }
        if (b.anzahl > 0) {
            detailSetzen(b, "%s steps higher than STEPSIZE %s on walkable ground (highest %s)", {std::to_string(b.anzahl), zahl(m_fig.stufe), zahl(hoechste)});
            m_out.push_back(b);
        }
    }

    // --- 4. Tueren und Gaenge -------------------------------------------------------------------------------------
    struct Weite {
        bool ok = false;     // man kann hier stehen (nicht im Festen)
        double hoehe = 0;    // bis zur Decke (hoechstens die Gangmasse + 1)
        double breite = 0;   // die engste Spanne waagerecht
        V achse{1, 0, 0};    // die Richtung der weitesten Spanne (der Gang)
    };
    Weite weiteBei(const V& p) const {
        Weite w;
        const double hMax = std::max(m_o.masse.gangHoehe, m_o.masse.tuerHoehe) + 1;
        const double bMax = std::max(m_o.masse.gangBreite, m_o.masse.tuerBreite) + 1;
        const Treffer h = m_w.strahl(p + V(0, 0, 0.1), V(0, 0, 1), hMax, Modus::Kollision, m_art);
        if (h.innen) { return w; }
        w.hoehe = h.ja ? h.t + 0.1 : hMax;
        const double z = p[2] - m_fig.unten + (m_fig.oben + m_fig.unten) * 0.5;   // Mitte der Spielfigur
        double eng = 1e30;
        double weit = -1;
        for (int k = 0; k < 4; ++k) {
            const double win = k * 3.14159265358979323846 / 4;
            const V d(std::cos(win), std::sin(win), 0);
            const V o(p[0], p[1], z);
            const Treffer a = m_w.strahl(o, d, bMax, Modus::Kollision, m_art);
            const Treffer c = m_w.strahl(o, d * -1.0, bMax, Modus::Kollision, m_art);
            if (a.innen || c.innen) { return w; }
            const double spanne = (a.ja ? a.t : bMax) + (c.ja ? c.t : bMax);
            eng = std::min(eng, spanne);
            if (spanne > weit) {
                weit = spanne;
                w.achse = d;
            }
        }
        w.breite = eng;
        w.ok = true;
        return w;
    }
    void durchgaenge() {
        static const char* const kQuelle =
            "Clone Wars list: doors at least 110 x 64, corridors at least 128 x 96 (height x width, adjustable); player box OpenJK "
            "code/game/g_client.cpp:55-58, code/game/bg_public.h:51-53 (crouched 40 high)";
        const CwMasse& m = m_o.masse;
        // a) Tueren: func_door, je Team zusammen (G_FindTeams: "team")
        Befund tuer = neu("tuerklein", Schwere::Warnung, kQuelle);
        std::map<std::string, std::vector<int>> teams;
        for (std::size_t e = 0; e < m_a.ents.size(); ++e) {
            if (klein(m_a.ents[e].klasse) != "func_door" || m_a.ents[e].prims.empty()) { continue; }
            const std::string t = m_a.ents[e].wert("team");
            teams[t.empty() ? "#" + std::to_string(e) : "team " + t].push_back(static_cast<int>(e));
        }
        for (const auto& [name, liste] : teams) {
            double mn[3] = {1e30, 1e30, 1e30};
            double mx[3] = {-1e30, -1e30, -1e30};
            for (const int e : liste) {
                for (const AbbildPrimitiv& pr : m_a.ents[static_cast<std::size_t>(e)].prims) {
                    bool ursprung = !pr.flaechen.empty();
                    for (const AbbildFlaeche& f : pr.flaechen) {
                        if (!m_art(f.shader).ursprung && klein(f.shader).find("/origin") == std::string::npos) { ursprung = false; }
                    }
                    if (ursprung) { continue; }
                    for (int a = 0; a < 3; ++a) {
                        mn[a] = std::min(mn[a], static_cast<double>(pr.mins[static_cast<std::size_t>(a)]));
                        mx[a] = std::max(mx[a], static_cast<double>(pr.maxs[static_cast<std::size_t>(a)]));
                    }
                }
            }
            if (mn[0] > mx[0]) { continue; }
            const double hoehe = mx[2] - mn[2];
            const double breite = std::max(mx[0] - mn[0], mx[1] - mn[1]);
            if (hoehe + 0.01 >= m.tuerHoehe && breite + 0.01 >= m.tuerBreite) { continue; }
            const int e = liste.front();
            ObjektRef r{e, -1, -1, stelle(e, -1) + (name[0] == '#' ? std::string() : " (" + name + ")") + ": " + zahl(hoehe) + " x " + zahl(breite)};
            punktSetzen(r, V((mn[0] + mx[0]) * 0.5, (mn[1] + mx[1]) * 0.5, mn[2]));
            add(tuer, r);
        }
        if (tuer.anzahl > 0) {
            detailSetzen(tuer, "%s doors smaller than %s x %s (height x width)", {std::to_string(tuer.anzahl), zahl(m.tuerHoehe), zahl(m.tuerBreite)});
            m_out.push_back(tuer);
        }
        // b) Durchgaenge und Gaenge auf den begehbaren Flaechen (Tueren und andere Mover stehen offen)
        m_w.ohneMover = true;
        Befund eng = neu("durchgangeng", Schwere::Warnung, kQuelle);
        Befund gang = neu("gangeng", Schwere::Hinweis, kQuelle);
        for (const Vol& v : m_w.vol) {
            if (!v.kollision || v.mover) { continue; }
            for (std::size_t s = 0; s < v.p->flaechen.size(); ++s) {
                const AbbildFlaeche& f = v.p->flaechen[s];
                const V n = f.ebene.normal();
                if (n[2] < m_fig.gehen || f.punkte.size() < 3) { continue; }
                double engsteTuer = 1e30;
                Weite wTuer;
                V pTuer;
                double engsterGang = 1e30;
                Weite wGang;
                V pGang;
                for (const V& p : proben(f.punkte, n, 32, 4096)) {
                    if (!erreichbarNahe(p, 24, 2, 2)) { continue; }
                    bool verdeckt = false;
                    if (bedeckt(p + V(0, 0, 0.25), verdeckt) > -1e29 || verdeckt) { continue; }
                    const Weite w = weiteBei(p);
                    if (!w.ok || w.hoehe < m_fig.hoeheGeduckt() || w.breite < 2 * m_fig.halb) { continue; }
                    const bool engGang = w.breite + 0.01 < m.gangBreite || w.hoehe + 0.01 < m.gangHoehe;
                    if (!engGang) { continue; }
                    // ein Gang: auch eine Gangbreite weiter in beide Richtungen eng
                    int engeSeiten = 0;
                    for (const double richtung : {1.0, -1.0}) {
                        const V q = p + w.achse * (m.gangBreite * richtung);
                        const Treffer boden = m_w.strahl(V(q[0], q[1], p[2] + m_fig.stufe + 0.5), V(0, 0, -1), 2 * m_fig.stufe + 1, Modus::Kollision, m_art);
                        if (!boden.ja || boden.innen) { continue; }
                        const Weite w2 = weiteBei(V(q[0], q[1], p[2] + m_fig.stufe + 0.5 - boden.t));
                        if (w2.ok && (w2.breite + 0.01 < m.gangBreite || w2.hoehe + 0.01 < m.gangHoehe)) { ++engeSeiten; }
                    }
                    const double mass = std::min(w.breite / m.gangBreite, w.hoehe / m.gangHoehe);
                    if (engeSeiten == 2) {
                        if (mass < engsterGang) {
                            engsterGang = mass;
                            wGang = w;
                            pGang = p;
                        }
                    } else if (w.breite + 0.01 < m.tuerBreite || w.hoehe + 0.01 < m.tuerHoehe) {
                        const double mt = std::min(w.breite / m.tuerBreite, w.hoehe / m.tuerHoehe);
                        if (mt < engsteTuer) {
                            engsteTuer = mt;
                            wTuer = w;
                            pTuer = p;
                        }
                    }
                }
                if (engsterGang < 1e29) {
                    ObjektRef r = flaechenRef(v, static_cast<int>(s));
                    r.text += ": corridor " + zahl(wGang.hoehe) + " x " + zahl(wGang.breite) + " at " + ort(pGang);
                    punktSetzen(r, pGang);
                    add(gang, r);
                }
                if (engsteTuer < 1e29) {
                    ObjektRef r = flaechenRef(v, static_cast<int>(s));
                    r.text += ": passage " + zahl(wTuer.hoehe) + " x " + zahl(wTuer.breite) + " at " + ort(pTuer);
                    punktSetzen(r, pTuer);
                    add(eng, r);
                }
            }
        }
        m_w.ohneMover = false;
        if (eng.anzahl > 0) {
            detailSetzen(eng, "%s places in passages lower or narrower than a door (%s x %s)", {std::to_string(eng.anzahl), zahl(m.tuerHoehe), zahl(m.tuerBreite)});
            m_out.push_back(eng);
        }
        if (gang.anzahl > 0) {
            detailSetzen(gang, "%s places in corridors lower or narrower than %s x %s", {std::to_string(gang.anzahl), zahl(m.gangHoehe), zahl(m.gangBreite)});
            m_out.push_back(gang);
        }
    }

    // --- 5. Texturen ---------------------------------------------------------------------------------------------
    void texturen() {
        // alle benutzten Shader: Flaechen, Patches, Modelle (nach Skin und _remap)
        std::map<std::string, ObjektRef> benutzt;   // klein -> erste Stelle
        for (std::size_t e = 0; e < m_a.ents.size(); ++e) {
            for (std::size_t k = 0; k < m_a.ents[e].prims.size(); ++k) {
                const AbbildPrimitiv& pr = m_a.ents[e].prims[k];
                const auto dazu = [&](const std::string& s) {
                    if (s.empty()) { return; }
                    benutzt.emplace(klein(s), ObjektRef{static_cast<int>(e), static_cast<int>(k), -1, stelle(static_cast<int>(e), static_cast<int>(k)) + " " + s});
                };
                if (pr.patch) { dazu(pr.shader); }
                for (const AbbildFlaeche& f : pr.flaechen) { dazu(f.shader); }
            }
        }
        std::set<std::string> modellShader;
        for (const AbbildModell& m : m_o.modelle) {
            if (!m.modell || m.modell->null) { continue; }
            for (std::size_t s = 0; s < m.modell->flaechen.size(); ++s) {
                const std::string sh = s < m.shader.size() ? m.shader[s] : m.modell->flaechen[s].shader;
                if (sh.empty()) { continue; }
                modellShader.insert(klein(sh));
                benutzt.emplace(klein(sh), ObjektRef{m.entitaet, -1, -1, stelle(m.entitaet, -1) + " " + m.pfad + ": " + sh});
            }
        }
        CwInventar inv;
        Befund npot = neu("bildnpot", Schwere::Fehler,
                          "OpenJK code/rd-vanilla/tr_image.cpp:1002-1004 R_CreateImage (ERR_FATAL \"dimensions not power of 2\"), Movie Duels "
                          "code/rd-vanilla/tr_image.cpp:1049; rd-common/tr_image_load.cpp:94-145 R_LoadImage (no resampling)");
        Befund groesse = neu("bildgroesse", Schwere::Hinweis,
                             "Clone Wars list: image sides 64 to 2048 pixels (adjustable); OpenJK code/rd-vanilla/tr_image.cpp:597 (larger than the "
                             "driver allows: halved until it fits)");
        Befund fehlt = neu("bildfehlt", Schwere::Warnung,
                           "OpenJK code/rd-vanilla/tr_shader.cpp ParseStage / ParseSkyParms (R_FindImageFile fails: \"could not find\", the default "
                           "image), R_FindShader (no script and no image: the default shader); q3map2 shaders.cpp (\"Couldn't find image\")");
        std::map<std::string, std::string> bildFuer;   // Bild -> erster Shader
        for (const auto& [name, ref] : benutzt) {
            inv.shader.push_back(name);
            const ShaderArt& sa = m_art(name);
            const bool common = name.rfind("textures/common/", 0) == 0 || name.rfind("textures/system/", 0) == 0;
            // Shader eines Modells ohne Skript sieht der Hauptfaden nicht nach (das Bild zu entpacken kostete dort Zeit): hier ist
            // der Name das Bild (R_FindShader -> R_FindImageFile), sein Kopf entscheidet, ob es da ist
            if (m_o.shaderBekanntGeprueft && m_o.shader.count(name) == 0 && modellShader.count(name) != 0) {
                int b = 0;
                int h = 0;
                if (m_o.bildMass && !m_o.bildMass(name, b, h)) {
                    inv.shaderFehlen.push_back(name);
                    ObjektRef r = ref;
                    r.text += " (shader)";
                    add(fehlt, r);
                } else {
                    bildFuer.emplace(name, name);
                }
                continue;
            }
            if (m_o.shaderBekanntGeprueft && !sa.bekannt && !common) {
                inv.shaderFehlen.push_back(name);
                if (modellShader.count(name) != 0) {   // die Shader der Flaechen meldet schon die Pruefung "textur"
                    ObjektRef r = ref;
                    r.text += " (shader)";
                    add(fehlt, r);
                }
                continue;
            }
            if (!sa.sichtbar || sa.werkzeug) { continue; }
            for (const std::string& bild : sa.bilder) { bildFuer.emplace(bild, name); }
        }
        if (m_o.bildMass) {
            for (const auto& [bild, shader] : bildFuer) {
                CwInventar::Bild x;
                x.name = bild;
                x.gefunden = m_o.bildMass(bild, x.breite, x.hoehe);
                inv.bilder.push_back(x);
                const ObjektRef& basis = benutzt.at(shader);
                ObjektRef r = basis;
                if (!x.gefunden) {
                    r.text = bild + " (" + shader + ")";
                    add(fehlt, r);
                    continue;
                }
                const auto zweierPotenz = [](int v) { return v > 0 && (v & (v - 1)) == 0; };
                r.text = bild + " " + std::to_string(x.breite) + " x " + std::to_string(x.hoehe) + " (" + shader + ")";
                if (!zweierPotenz(x.breite) || !zweierPotenz(x.hoehe)) {
                    add(npot, r);
                } else if (std::min(x.breite, x.hoehe) < m_o.masse.bildMin || std::max(x.breite, x.hoehe) > m_o.masse.bildMax) {
                    add(groesse, r);
                }
            }
        }
        if (npot.anzahl > 0) {
            detailSetzen(npot, "%s images with a side that is not a power of two (the game stops with ERR_FATAL)", {std::to_string(npot.anzahl)});
            m_out.push_back(npot);
        }
        if (fehlt.anzahl > 0) {
            detailSetzen(fehlt, "%s images or model shaders the game cannot find", {std::to_string(fehlt.anzahl)});
            m_out.push_back(fehlt);
        }
        if (groesse.anzahl > 0) {
            detailSetzen(groesse, "%s images smaller than %s or larger than %s pixels",
                         {std::to_string(groesse.anzahl), std::to_string(m_o.masse.bildMin), std::to_string(m_o.masse.bildMax)});
            m_out.push_back(groesse);
        }
        if (m_o.inventar != nullptr) { *m_o.inventar = std::move(inv); }
    }

    // --- 6. Modelle --------------------------------------------------------------------------------------------
    void modelle() {
        Befund welt = neu("modellweltkoord", Schwere::Warnung,
                          "q3map2 portals.cpp:536-596 FloodEntities (every point entity counts at its origin + 1: in the void a leak, in solid "
                          "\"Entity in solid\"), model.cpp:1581 InsertModel (the geometry at origin + vertex); Movie Duels code/cgame/cg_main.cpp:"
                          "3263-3290 CG_DrawMiscEnts (misc_model_static: PVS of origin + zoffset + 1 and the distance from the origin)");
        Befund flaeche = neu("modellteilflaeche", Schwere::Fehler,
                             "OpenJK code/rd-vanilla/tr_model.cpp:899-906 R_LoadMD3 (ERR_DROP above SHADER_MAX_VERTEXES / SHADER_MAX_INDEXES), "
                             "tr_surface.cpp:53-69 RB_CheckOverflow, code/qcommon/qfiles.h:33-34; q3map2 splits baked misc_model (model.cpp:331 "
                             "AI_CONFIG_PP_SLM_VERTEX_LIMIT)");
        Befund md3 = neu("md3grenzen", Schwere::Hinweis,
                         "OpenJK code/qcommon/qfiles.h:101-105 MD3_MAX_TRIANGLES 8192, MD3_MAX_VERTS 4096, MD3_MAX_SURFACES 32 + 32 (format limits: "
                         "R_LoadMD3 does not check them); assimp MD3Loader.cpp:405-419 (q3map2 warns \"Quake III ... limit exceeded\")");
        const Grenze* gv = grenzeSuchen(m_o.spiel, "renderer.flaechenpunkte");
        const Grenze* gi = grenzeSuchen(m_o.spiel, "renderer.flaechenindizes");
        const long long maxPunkte = gv != nullptr ? gv->erlaubt : 999;
        const long long maxIndizes = gi != nullptr ? gi->erlaubt : 5999;
        std::set<std::string> md3Gemeldet;
        double weltMn[3] = {1e30, 1e30, 1e30};   // der Kasten aller Pinsel der Welt (worldspawn, func_group)
        double weltMx[3] = {-1e30, -1e30, -1e30};
        for (const AbbildEntitaet& en : m_a.ents) {
            if (en.klasse != "worldspawn" && en.klasse != "func_group") { continue; }
            for (const AbbildPrimitiv& pr : en.prims) {
                for (int a = 0; a < 3; ++a) {
                    weltMn[a] = std::min(weltMn[a], static_cast<double>(pr.mins[static_cast<std::size_t>(a)]));
                    weltMx[a] = std::max(weltMx[a], static_cast<double>(pr.maxs[static_cast<std::size_t>(a)]));
                }
            }
        }
        for (std::size_t i = 0; i < m_o.modelle.size(); ++i) {
            const AbbildModell& m = m_o.modelle[i];
            if (!m.modell || m.modell->null || m.entitaet < 0 || m.entitaet >= static_cast<int>(m_a.ents.size())) { continue; }
            const AbbildEntitaet& en = m_a.ents[static_cast<std::size_t>(m.entitaet)];
            const auto& k = m_modellKasten[i];
            if (k[0] <= k[3]) {
                float o[3] = {0, 0, 0};
                (void)std::sscanf(en.wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
                double d2 = 0;
                double groesste = 0;
                for (int a = 0; a < 3; ++a) {
                    const double v = o[a];
                    const double unter = k[static_cast<std::size_t>(a)] - v;
                    const double ueber = v - k[static_cast<std::size_t>(a + 3)];
                    if (unter > 0) { d2 += unter * unter; }
                    if (ueber > 0) { d2 += ueber * ueber; }
                    groesste = std::max(groesste, k[static_cast<std::size_t>(a + 3)] - k[static_cast<std::size_t>(a)]);
                }
                const double d = std::sqrt(d2);
                // ein eingebackenes misc_model stoert nur, wenn sein Ursprung ausserhalb aller Pinsel der Welt liegt (dort ist das
                // Leere: FloodEntities findet ein Leck); das Spiel blendet Laufzeit-Modelle immer nach dem Ursprung aus
                const bool imLeeren = o[0] < weltMn[0] || o[1] < weltMn[1] || o[2] < weltMn[2] || o[0] > weltMx[0] || o[1] > weltMx[1] || o[2] > weltMx[2];
                if (d > std::max(m_o.masse.toleranz, groesste) && (!m.eingebacken || imLeeren)) {
                    ObjektRef r{m.entitaet, -1, -1,
                                stelle(m.entitaet, -1) + " " + m.pfad + ": origin " + ort(V(o[0], o[1], o[2])) + ", geometry " + zahl(d) + " units away" +
                                    (m.eingebacken ? std::string(", origin outside the map (leak)") : std::string())};
                    punktSetzen(r, V((k[0] + k[3]) * 0.5, (k[1] + k[4]) * 0.5, (k[2] + k[5]) * 0.5));
                    add(welt, r);
                }
            }
            const std::string p = klein(m.pfad);
            const bool istMd3 = p.size() > 4 && p.compare(p.size() - 4, 4, ".md3") == 0;
            if (!istMd3) { continue; }
            if (!m.eingebacken) {   // das Spiel laedt sie zur Laufzeit (R_LoadMD3)
                for (std::size_t s = 0; s < m.modell->flaechen.size(); ++s) {
                    const ModellFlaeche& f = m.modell->flaechen[s];
                    const long long punkte = static_cast<long long>(f.punkte.size());
                    const long long indizes = static_cast<long long>(f.indizes.size());
                    if (punkte <= maxPunkte && indizes <= maxIndizes) { continue; }
                    add(flaeche, ObjektRef{m.entitaet, -1, -1,
                                           stelle(m.entitaet, -1) + " " + m.pfad + ", surface " + std::to_string(s) + ": " + std::to_string(punkte) + " vertices, " +
                                               std::to_string(indizes) + " indexes"});
                    break;
                }
            }
            if (md3Gemeldet.count(p) != 0) { continue; }
            const std::size_t flaechen = m.modell->flaechen.size();
            std::size_t meistePunkte = 0;
            std::size_t meisteDreiecke = 0;
            for (const ModellFlaeche& f : m.modell->flaechen) {
                meistePunkte = std::max(meistePunkte, f.punkte.size());
                meisteDreiecke = std::max(meisteDreiecke, f.indizes.size() / 3);
            }
            if (flaechen > 64 || meistePunkte > 4096 || meisteDreiecke > 8192) {
                md3Gemeldet.insert(p);
                add(md3, ObjektRef{m.entitaet, -1, -1,
                                   stelle(m.entitaet, -1) + " " + m.pfad + ": " + std::to_string(flaechen) + " surfaces, up to " + std::to_string(meistePunkte) +
                                       " vertices and " + std::to_string(meisteDreiecke) + " triangles"});
            }
        }
        if (flaeche.anzahl > 0) {
            detailSetzen(flaeche, "%s models with a surface over %s vertices or %s indexes (the game stops with ERR_DROP)",
                         {std::to_string(flaeche.anzahl), std::to_string(maxPunkte), std::to_string(maxIndizes)});
            m_out.push_back(flaeche);
        }
        if (welt.anzahl > 0) {
            detailSetzen(welt, "%s models whose geometry lies away from their origin (exported in world coordinates?)", {std::to_string(welt.anzahl)});
            m_out.push_back(welt);
        }
        if (md3.anzahl > 0) {
            detailSetzen(md3, "%s .md3 files over the format limits of qfiles.h (64 surfaces, 4096 vertices, 8192 triangles)", {std::to_string(md3.anzahl)});
            m_out.push_back(md3);
        }
    }

    bool clipModellDa(std::size_t i) const {
        for (const Dr& d : m_w.dr) {
            if (d.modell == static_cast<int>(i) && d.kollision) { return true; }
        }
        return false;
    }

    // --- 7. Requisiten: schwebend oder im Boden ---------------------------------------------------------------------
    void requisiten() {
        static const char* const kQuelle =
            "Clone Wars list: nothing floats, props stand on the floor (at most 1 unit, adjustable); q3map2 surface.cpp AddReferenceToLeaf (baked "
            "triangles inside solid are dropped)";
        Befund schwebt = neu("requisitschwebt", Schwere::Warnung, kQuelle);
        Befund steckt = neu("requisitsteckt", Schwere::Hinweis, kQuelle);
        const double tol = m_o.masse.toleranz;
        double tiefste = 0;
        for (std::size_t i = 0; i < m_o.modelle.size(); ++i) {
            const AbbildModell& m = m_o.modelle[i];
            const auto& k = m_modellKasten[i];
            if (!m.modell || m.modell->null || k[0] > k[3]) { continue; }
            const double mn[3] = {k[0] - tol, k[1] - tol, k[2] - tol};
            const double mx[3] = {k[3] + tol, k[4] + tol, k[5] + tol};
            bool beruehrt = false;
            m_w.imKasten(
                mn, mx,
                [&](const Vol& v) {
                    if (beruehrt || !v.kollision) { return; }
                    if (v.mn[0] > mx[0] || v.mx[0] < mn[0] || v.mn[1] > mx[1] || v.mx[1] < mn[1] || v.mn[2] > mx[2] || v.mx[2] < mn[2]) { return; }
                    for (const AbbildFlaeche& f : v.p->flaechen) {   // Trennebene: alle Ecken des Kastens davor?
                        bool alleDavor = true;
                        for (int c = 0; c < 8 && alleDavor; ++c) {
                            const V e((c & 1) ? mx[0] : mn[0], (c & 2) ? mx[1] : mn[1], (c & 4) ? mx[2] : mn[2]);
                            if (abstandZuPunkt(f.ebene, e) <= 0) { alleDavor = false; }
                        }
                        if (alleDavor) { return; }
                    }
                    beruehrt = true;
                },
                [&](const Dr& d, int) {
                    if (beruehrt || !d.kollision || d.modell == static_cast<int>(i)) { return; }
                    const float tx0 = std::min({d.a[0], d.b[0], d.c[0]});
                    const float tx1 = std::max({d.a[0], d.b[0], d.c[0]});
                    const float ty0 = std::min({d.a[1], d.b[1], d.c[1]});
                    const float ty1 = std::max({d.a[1], d.b[1], d.c[1]});
                    const float tz0 = std::min({d.a[2], d.b[2], d.c[2]});
                    const float tz1 = std::max({d.a[2], d.b[2], d.c[2]});
                    if (tx0 > mx[0] || tx1 < mn[0] || ty0 > mx[1] || ty1 < mn[1] || tz0 > mx[2] || tz1 < mn[2]) { return; }
                    beruehrt = true;
                });
            // ein anderes Modell, auf dem es steht (Vase auf dem Tisch)
            for (std::size_t j = 0; j < m_o.modelle.size() && !beruehrt; ++j) {
                const auto& k2 = m_modellKasten[j];
                if (j == i || k2[0] > k2[3]) { continue; }
                if (k2[0] <= mx[0] && k2[3] >= mn[0] && k2[1] <= mx[1] && k2[4] >= mn[1] && k2[2] <= mx[2] && k2[5] >= mn[2]) { beruehrt = true; }
            }
            const V boden((k[0] + k[3]) * 0.5, (k[1] + k[4]) * 0.5, k[2]);
            if (!beruehrt) {
                const Treffer t = m_w.strahl(boden, V(0, 0, -1), 65536, Modus::Kollision, m_art);
                ObjektRef r{m.entitaet, -1, -1, stelle(m.entitaet, -1) + " " + m.pfad + ": " + (t.ja ? zahl(t.t) + " units above the floor" : std::string("no floor below"))};
                punktSetzen(r, boden);
                add(schwebt, r);
                continue;
            }
            // im Boden: von der Mitte des Modells hinab liegt der erste Boden ueber seiner Unterkante. Steckt schon die Mitte in der
            // Kollision, ist es in eine Wand oder Decke eingelassen (Lampe, Tafel) - keine Requisite auf dem Boden. Ein Modell mit
            // eigener Kollision (Clip) steht auf seinen eigenen Dreiecken: nicht geprueft.
            if (m.eingebacken && clipModellDa(i)) { continue; }
            const V mitte((k[0] + k[3]) * 0.5, (k[1] + k[4]) * 0.5, (k[2] + k[5]) * 0.5);
            bool innen = false;
            m_w.enthaltend(mitte, 0.01, [&](const Vol& v) {
                if (v.kollision) { innen = true; }
            });
            if (innen) { continue; }
            const Treffer t = m_w.strahl(mitte, V(0, 0, -1), mitte[2] - k[2] + 1, Modus::Kollision, m_art);
            if (!t.ja || t.innen || t.n[2] < m_fig.gehen) { continue; }
            const double tiefe = (mitte[2] - t.t) - k[2];
            if (tiefe > tol) {
                ObjektRef r{m.entitaet, -1, -1, stelle(m.entitaet, -1) + " " + m.pfad + ": " + zahl(tiefe) + " units in the floor"};
                punktSetzen(r, boden);
                add(steckt, r);
                tiefste = std::max(tiefste, tiefe);
            }
        }
        if (schwebt.anzahl > 0) {
            detailSetzen(schwebt, "%s props float in the air (they touch no collision within %s units)", {std::to_string(schwebt.anzahl), zahl(tol)});
            m_out.push_back(schwebt);
        }
        if (steckt.anzahl > 0) {
            detailSetzen(steckt, "%s props reach more than %s units into the floor (deepest %s)", {std::to_string(steckt.anzahl), zahl(tol), zahl(tiefste)});
            m_out.push_back(steckt);
        }
    }
};

}  // namespace

void pruefeClonewars(const KartenAbbild& a, const PruefOptionen& o, std::vector<Befund>& out, const std::function<const ShaderArt&(const std::string&)>& art) {
    Cw(a, o, out, art).alle();
}

void clonewarsInventar(const KartenAbbild& a, const PruefOptionen& o, const std::function<const ShaderArt&(const std::string&)>& art) {
    if (o.inventar == nullptr) { return; }
    std::vector<Befund> weg;   // die Befunde der Texturpruefung zaehlen ohne den Schalter nicht
    Cw(a, o, weg, art).nurInventar();
}

}  // namespace bhed::karte
